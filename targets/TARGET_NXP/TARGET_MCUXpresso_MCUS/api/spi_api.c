/* mbed Microcontroller Library
 * Copyright (c) 2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <math.h>
#include "mbed_assert.h"

#include "spi_api.h"

#if DEVICE_SPI

#include "cmsis.h"
#include "pinmap.h"
#include "mbed_error.h"
#include "fsl_spi.h"
#include "PeripheralPins.h"

/* Array of SPI peripheral base address. */
static SPI_Type *const spi_address[] = SPI_BASE_PTRS;

void spi_init(spi_t *obj, PinName mosi, PinName miso, PinName sclk, PinName ssel)
{
    // determine the SPI to use
    uint32_t spi_mosi = pinmap_peripheral(mosi, PinMap_SPI_MOSI);
    uint32_t spi_miso = pinmap_peripheral(miso, PinMap_SPI_MISO);
    uint32_t spi_sclk = pinmap_peripheral(sclk, PinMap_SPI_SCLK);
    uint32_t spi_ssel = pinmap_peripheral(ssel, PinMap_SPI_SSEL);
    uint32_t spi_data = pinmap_merge(spi_mosi, spi_miso);
    uint32_t spi_cntl = pinmap_merge(spi_sclk, spi_ssel);

    obj->instance = pinmap_merge(spi_data, spi_cntl);
    MBED_ASSERT((int)obj->instance != NC);

    // pin out the spi pins
    pinmap_pinout(mosi, PinMap_SPI_MOSI);
    pinmap_pinout(miso, PinMap_SPI_MISO);
    pinmap_pinout(sclk, PinMap_SPI_SCLK);
    if (ssel != NC) {
        pinmap_pinout(ssel, PinMap_SPI_SSEL);
    }
}

void spi_free(spi_t *obj)
{
    SPI_Deinit(spi_address[obj->instance]);
}

void spi_format(spi_t *obj, int bits, int mode, int slave)
{
    spi_master_config_t master_config;
    spi_slave_config_t slave_config;

    /* Bits: values between 4 and 16 are valid */
    MBED_ASSERT(bits >= 4 && bits <= 16);
    obj->bits = bits;

    if (slave) {
        /* Slave config */
        SPI_SlaveGetDefaultConfig(&slave_config);
        slave_config.dataWidth = (uint32_t)bits - 1;
        slave_config.polarity = (mode & 0x2) ? kSPI_ClockPolarityActiveLow : kSPI_ClockPolarityActiveHigh;
        slave_config.phase = (mode & 0x1) ? kSPI_ClockPhaseSecondEdge : kSPI_ClockPhaseFirstEdge;

        SPI_SlaveInit(spi_address[obj->instance], &slave_config);
    } else {
        /* Master config */
        SPI_MasterGetDefaultConfig(&master_config);
        master_config.dataWidth = (uint32_t)bits - 1;
        master_config.polarity = (mode & 0x2) ? kSPI_ClockPolarityActiveLow : kSPI_ClockPolarityActiveHigh;
        master_config.phase = (mode & 0x1) ? kSPI_ClockPhaseSecondEdge : kSPI_ClockPhaseFirstEdge;
        master_config.direction = kSPI_MsbFirst;

        switch (obj->instance) {
            case 0:
                CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);
                RESET_PeripheralReset(kFC0_RST_SHIFT_RSTn);
                break;
            case 1:
                CLOCK_AttachClk(kFRO12M_to_FLEXCOMM1);
                RESET_PeripheralReset(kFC1_RST_SHIFT_RSTn);
                break;
            case 2:
                CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);
                RESET_PeripheralReset(kFC2_RST_SHIFT_RSTn);
                break;
            case 3:
                CLOCK_AttachClk(kFRO12M_to_FLEXCOMM3);
                RESET_PeripheralReset(kFC3_RST_SHIFT_RSTn);
                break;
            case 4:
                CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);
                RESET_PeripheralReset(kFC4_RST_SHIFT_RSTn);
                break;
            case 5:
                CLOCK_AttachClk(kFRO12M_to_FLEXCOMM5);
                RESET_PeripheralReset(kFC5_RST_SHIFT_RSTn);
                break;
            case 6:
                CLOCK_AttachClk(kFRO12M_to_FLEXCOMM6);
                RESET_PeripheralReset(kFC6_RST_SHIFT_RSTn);
                break;
            case 7:
                CLOCK_AttachClk(kFRO12M_to_FLEXCOMM7);
                RESET_PeripheralReset(kFC7_RST_SHIFT_RSTn);
                break;
#if (FSL_FEATURE_SOC_FLEXCOMM_COUNT > 8U)
            case 8:
                CLOCK_AttachClk(kFRO12M_to_FLEXCOMM8);
                RESET_PeripheralReset(kFC8_RST_SHIFT_RSTn);
                break;
#endif
#if (FSL_FEATURE_SOC_FLEXCOMM_COUNT > 9U)
            case 9:
                CLOCK_AttachClk(kFRO12M_to_FLEXCOMM9);
                RESET_PeripheralReset(kFC9_RST_SHIFT_RSTn);
                break;
#endif

        }

        SPI_MasterInit(spi_address[obj->instance], &master_config, 12000000);
    }
}

void spi_frequency(spi_t *obj, int hz)
{
    SPI_MasterSetBaud(spi_address[obj->instance], (uint32_t)hz, 12000000);
}

static inline int spi_readable(spi_t * obj)
{
    return (SPI_GetStatusFlags(spi_address[obj->instance]) & kSPI_RxNotEmptyFlag);
}

int spi_master_write(spi_t *obj, int value)
{
    uint32_t rx_data;

    SPI_WriteData(spi_address[obj->instance], (uint16_t)value, kSPI_FrameAssert);
    // wait rx buffer full
    while (!spi_readable(obj));
    rx_data = SPI_ReadData(spi_address[obj->instance]);
    return rx_data & 0xffff;
}

int spi_master_block_write(spi_t *obj, const char *tx_buffer, int tx_length, 
                           char *rx_buffer, int rx_length, char write_fill) {
    int total = (tx_length > rx_length) ? tx_length : rx_length;

    for (int i = 0; i < total; i++) {
        char out = (i < tx_length) ? tx_buffer[i] : write_fill;
        char in = spi_master_write(obj, out);
        if (i < rx_length) {
            rx_buffer[i] = in;
        }
    }

    return total;
}

int spi_slave_receive(spi_t *obj)
{
    return spi_readable(obj);
}

int spi_slave_read(spi_t *obj)
{
    uint32_t rx_data;

    while (!spi_readable(obj));
    rx_data = SPI_ReadData(spi_address[obj->instance]);
    return rx_data & 0xffff;
}

void spi_slave_write(spi_t *obj, int value)
{
    SPI_WriteData(spi_address[obj->instance], (uint16_t)value, 0);
}

#endif
