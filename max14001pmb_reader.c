/*  max14001pmb_reader.c - Program to get ADC readings for the two 
 *  MAX14001 devices on the MAX14001PMB Evaluation Board 
 *
 *  Copyright (c) 2025 Marilene Andrade Garcia <marilene.agarcia@gmail.com>
 *
 *  Datasheet MAX14001: https://www.analog.com/media/en/technical-documentation/data-sheets/MAX14001-MAX14002.pdf
 *  Datasheet MAX14001PMB: https://www.analog.com/media/en/technical-documentation/data-sheets/MAX14001PMB.pdf 
 * 
 *  This code was written based on the formulas obtained from the
 *  MAX14001PMB circuit analysis, which is available at the link:
 *  https://github.com/MarileneGarcia/max14001pmb_reader/blob/main/MAX14001PMB_circuit_analysis.pdf
 *       
 *  MAX14001PMB Reader is free software: you can redistribute it and/or 
 *  modify it under the terms of the GNU General Public License as 
 *  published by the Free Software Foundation, either version 3 of the 
 *  License, or (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful, but 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 *  General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *      
 *  This file was partially generated with the assistance of ChatGPT (OpenAI)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <termios.h>

/* global flag to control threads */
volatile int running = 1;

/*  According to the board’s circuit configuration the U11 device measures 
 *  voltage with a offset that allows it to measure negative values. */
#define MAX14001_U11_ADC "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"
#define MAX14001_U11_FADC "/sys/bus/iio/devices/iio:device0/in_voltage0_mean_raw"

/*  According to the board’s circuit configuration the U51 device measures 
*   current with an offset that allows it to measure negative values. */
#define MAX14001_U51_ADC "/sys/bus/iio/devices/iio:device1/in_voltage0_raw"
#define MAX14001_U51_FADC "/sys/bus/iio/devices/iio:device1/in_voltage0_mean_raw"

typedef struct {
    int device_id;
    const char *path;
} max14001_params_t;

/* Read MAX14001 ADC values */
void *read_max14001(void *arg) {
    max14001_params_t *params = (max14001_params_t *)arg;
    int device_id = params->device_id;
    const char *path = params->path;
    char buf[64], error[64];
    int fd, max14001_val;
    ssize_t len;
    double input_current = 0, input_voltage = 0;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        snprintf(error, sizeof(error), "Failed to open %s", path);
        perror(error);
        pthread_exit(NULL);
    }

    len = read(fd, buf, sizeof(buf) - 1);
    if (len < 0) {
        snprintf(error, sizeof(error), "Failed to read %s", path);
        perror(error);
        pthread_exit(NULL);
    }

    buf[len] = '\0';
    max14001_val = atoi(buf);

    if(device_id) {
        input_current = ((max14001_val * 0.001220703125)-0.625)*10;
        printf("(%s): Input Current = %lf (A)\n", path, input_current);
    } else {
        input_voltage = (max14001_val - 511.06305173)/1.499118283;
        printf("(%s): Input Voltage = %lf (V)\n", path, input_voltage);
    }

    close(fd);

    return NULL;
}

/* Non-blocking keyboard check */
int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

/* Keyboard monitor thread */
void *keyboard_monitor(void *arg) {
    (void)arg;

    while (running) {
        if (kbhit()) {
            /* Signal all threads to stop */
            running = 0; 
        }
        /* Small delay to reduce CPU usage */
        usleep(100000); 
    }

    return NULL;
}

int main(void) {
    pthread_t t1, t2, t3, t4, tkb;
    int loop = 0;

    max14001_params_t max_u11_adc = {0, MAX14001_U11_ADC};
    max14001_params_t max_u11_fadc = {0, MAX14001_U11_FADC};
    max14001_params_t max_u51_adc = {1, MAX14001_U51_ADC};
    max14001_params_t max_u51_fadc = {1, MAX14001_U51_FADC};

    /* Press any key to stop the MAX14001 readings */
    pthread_create(&tkb, NULL, keyboard_monitor, NULL);
    printf("Press any key to stop the MAX14001 readings\n");

    while (running) {
        printf("Reading.. loop(%d)\n", loop);

        /* Start ADC reading threads */
        if (pthread_create(&t1, NULL, read_max14001, &max_u11_adc) != 0) {
            perror("pthread_create t1 fail");
            return 1;
        }

        if (pthread_create(&t2, NULL, read_max14001, &max_u11_fadc) != 0) {
            perror("pthread_create t2 fail");
            return 1;
        }

        if (pthread_create(&t3, NULL, read_max14001, &max_u51_adc) != 0) {
            perror("pthread_create t3 fail");
            return 1;
        }

        if (pthread_create(&t4, NULL, read_max14001, &max_u51_fadc) != 0) {
            perror("pthread_create t4 fail");
            return 1;
        }

        /* Wait for them to finish */
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        pthread_join(t3, NULL);
        pthread_join(t4, NULL);

        loop += 1;
        printf("\n\n");
        usleep(500000);
    }

    /* Wait for keyboard thread to finish */
    pthread_join(tkb, NULL);

    printf("MAX14001PMB Reader Program terminated.\n");
    return 0;
}
