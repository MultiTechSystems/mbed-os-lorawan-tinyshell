/* Library for SPI flash 25* devices.
 * Copyright (c) 2014 Multi-Tech Systems
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "SpiFlash25.h"

SpiFlash25::SpiFlash25(PinName mosi, PinName miso, PinName sclk, PinName cs, PinName W, PinName HOLD, int page_size, int mem_size)
:   _spi(mosi, miso, sclk),
    _cs(cs),
    _mem_size(mem_size),
    _page_size(page_size)
{

    _cs.write(1);
    _spi.format(8, 3);
    _spi.frequency(75000000);

    if (W != NC) {
        _w = new DigitalOut(W);
        _w->write(1);
    }
    if (HOLD != NC) {
        _hold = new DigitalOut(HOLD);
        _hold->write(1);
    }

    wakeup();
}

void SpiFlash25::format(int bits, int mode) {
    _spi.format(bits, mode);
}

void SpiFlash25::frequency(int hz) {
    _spi.frequency(hz);
}

bool SpiFlash25::read(int addr, int len, char* data) {
    if (addr + len > _mem_size) {
        return false;
    }

    enable_write();

    _cs.write(0);
    _spi.write(READ_DATA);
    _spi.write(high_byte(addr));
    _spi.write(mid_byte(addr));
    _spi.write(low_byte(addr));

    for (int i = 0; i < len; i++) {
        data[i] = _spi.write(0x00);
    }

    _cs.write(1);

    return true;
}

bool SpiFlash25::write(int addr, int len, const char* data) {
    if (addr + len > _mem_size) {
        return false;
    }

    int written = 0;
    int write_size = 0;

    while (written < len) {
        write_size = _page_size - ((addr + written) % _page_size);
        if (written + write_size > len) {
            write_size = len - written;
        }

        if (! write_page(addr + written, write_size, data + written)) {
            return false;
        }

        written += write_size;
    }

    return true;
}

char* SpiFlash25::read_id() {
    _cs.write(0);
    _spi.write(READ_IDENTIFICATION);
    _id[ID_MANUFACTURER] = _spi.write(0x00);
    _id[ID_MEM_TYPE] = _spi.write(0x00);
    _id[ID_MEM_SIZE] = _spi.write(0x00);
    _cs.write(1);

    return _id;
}

void SpiFlash25::write_status(char data) {
    enable_write();	
    _cs.write(0);
    _spi.write(WRITE_STATUS);
    _spi.write(data);	
    _cs.write(1);
    wait_for_write();    
}

char SpiFlash25::read_status() {
    char status;

    _cs.write(0);
    _spi.write(READ_STATUS);
    status = _spi.write(0x00);
    _cs.write(1);

    return status;
}

void SpiFlash25::clear_sector(int addr) {
    enable_write();

    _cs.write(0);
    _spi.write(SECTOR_ERASE);
    _spi.write(high_byte(addr));
    _spi.write(mid_byte(addr));
    _spi.write(low_byte(addr));
    _cs.write(1);

    wait_for_write();
}

void SpiFlash25::clear_mem() {
    enable_write();

    _cs.write(0);
    _spi.write(BULK_ERASE);
    _cs.write(1);

    wait_for_write();
}

bool SpiFlash25::write_page(int addr, int len, const char* data) {
    enable_write();

    _cs.write(0);
    _spi.write(PAGE_PROGRAM);
    _spi.write(high_byte(addr));
    _spi.write(mid_byte(addr));
    _spi.write(low_byte(addr));

    for (int i = 0; i < len; i++) {
        _spi.write(data[i]);
    }

    _cs.write(1);
    wait_for_write();

    return true;
}

void SpiFlash25::enable_write() {
    _cs.write(0);
    _spi.write(WRITE_ENABLE);
    _cs.write(1);
}

void SpiFlash25::wait_for_write() {
    while (read_status() & STATUS_WIP) {
        wait_us(10);
    }
}

void SpiFlash25::deep_power_down() {
    _cs.write(0);
    _spi.write(DEEP_POWER_DOWN);
    _cs.write(1);
}

void SpiFlash25::wakeup() {
    _cs.write(0);
    _spi.write(DEEP_POWER_DOWN_RELEASE);
    _cs.write(1);
}

