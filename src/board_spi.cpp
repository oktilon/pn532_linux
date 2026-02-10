#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "Pn532.h"
#include "board_spi.h"
#include "main.h"

static int spiFd = 0;

int open_spi(void) {
    int r;
    int fd = open("/dev/spidev0.0", O_RDWR);
    if (fd < 0) {
        log_err ("Open SPI error: %m");
        return -1;
    }

    int mode = SPI_MODE_0;
    r = ioctl(fd, SPI_IOC_WR_MODE, &mode);
    if (r < 0) {
        log_err ("Set SPI write mode error: %m");
        close(fd);
        return -1;
    }

    r = ioctl(fd, SPI_IOC_RD_MODE, &mode);
    if (r < 0) {
        log_err ("Set SPI read mode error: %m");
        close(fd);
        return -1;
    }

    int speed = 2300000;
    r = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    if (r < 0) {
        log_err ("Set SPI speed error: %m");
        close(fd);
        return -1;
    }

    spiFd = fd;
    return 0;
}

void close_spi(void) {
    if (spiFd > 0) {
        int r = close (spiFd);
        if (r < 0) {
            log_wrn ("Close SPI error: %m");
        }
    }
}

int send_spi(const uint8_t *sendBuf, uint8_t sendSize) {
    static uint8_t rxBuf[1024] = {0};
    struct spi_ioc_transfer tx = {
        .tx_buf = (unsigned long)sendBuf,
        .rx_buf = (unsigned long)rxBuf,
        .len = sendSize
    };

    if (spiFd > 0) {
        int r = ioctl(spiFd, SPI_IOC_MESSAGE(1), &tx);
        if (r < 0) {
            log_err ("Send to SPI error: %m");
            return -1;
        }
        log_trc ("Sent: %s [got: %s]"
            , Pn532::PrintHex(sendBuf, sendSize)
            , Pn532::PrintHex(rxBuf, sendSize));
        return 0;
    }
    return -1;
}

int read_spi(uint8_t *recBuf, uint8_t recSize, uint8_t sendByte) {
    static char txBuf[1024] = {0};
    int ret = -1;
    memset(txBuf, sendByte, recSize);
    struct spi_ioc_transfer tx = {
        .tx_buf = (unsigned long)txBuf,
        .rx_buf = (unsigned long)recBuf,
        .len = recSize
    };

    if (spiFd > 0) {
        int r = ioctl(spiFd, SPI_IOC_MESSAGE(1), &tx);
        if (r < 0) {
            log_err ("Read(+send) from SPI error: %m");
        } else {
            log_trc ("Got: %s", Pn532::PrintHex(recBuf, recSize));
            ret = 0;
        }
    }
    return ret;
}

bool write_then_read(const uint8_t *write_buffer, size_t write_len, uint8_t *read_buffer, size_t read_len, uint8_t sendvalue) {
    int r = send_spi (write_buffer, write_len);
    if (r == 0) {
        r = read_spi(read_buffer, read_len, sendvalue);
        return r == 0;
    }
    return 0;
}

void exportGpio(int gpio){
    FILE *fptr;

    fptr = fopen("/sys/class/gpio/export", "w");
    if (fptr) {
        fprintf(fptr, "%d", gpio);
        fclose(fptr);
    } else {
        log_wrn ("Export pin error: %m");
    }
}

void unexportGpio(int gpio){
    FILE *fptr;

    fptr = fopen("/sys/class/gpio/unexport", "w");
    if (fptr) {
        fprintf(fptr, "%d", gpio);
        fclose(fptr);
    } else {
        log_wrn ("Unexport pin error: %m");
    }
}

void setGpioDir(int gpio, int isOut){
    FILE *fptr;
    char buf[120];

    sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
    fptr = fopen(buf, "w");
    if (fptr) {
        if(isOut){
            fprintf(fptr, "%s", "out");
        } else {
            fprintf(fptr, "%s", "in");
        }
        fclose(fptr);
    } else {
        log_wrn ("Set pin direction error: %m");
    }
}

void setGpioOutput(int gpio, int isOn) {
    FILE *fptr;
    char buf[120];

    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    fptr = fopen(buf, "w");

    if (fptr) {
        if(isOn) {
            fprintf(fptr, "1");
        }else{
            fprintf(fptr, "0");
        }
        fclose(fptr);
    } else {
        log_wrn ("Set pin value error: %m");
    }
}

int board_init () {
    int r = open_spi ();
    if (r < 0) return r;
    // r = exportGpio (1);
    return 0;
}

void board_select (int /*isOn*/) {
    // setGpioOutput (1, isOn);
}