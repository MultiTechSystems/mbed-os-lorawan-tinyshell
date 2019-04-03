/**********************************************************************
* COPYRIGHT 2019 MULTI-TECH SYSTEMS, INC.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*   3. Neither the name of MULTI-TECH SYSTEMS, INC. nor the names of its contributors
*      may be used to endorse or promote products derived from this software
*      without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/

#include "config.h"

#if defined (TARGET_MTS_MDOT_F411RE)
char ConfigManager::file[] = "lora.cfg";
char ConfigManager::protected_file[] = "mdot.cfg";
char ConfigManager::session_file[] = "lora.session";
char ConfigManager::app_settings_file[] = "app.settings";
char ConfigManager::user_dir[] = "user";

Mutex mutex;
SpiFlash25 ConfigManager::_flash(SPI3_MOSI, SPI3_MISO, SPI3_SCK, SPI3_CS, FLASH_WP, FLASH_HOLD);

u8_t ConfigManager::spiffs_work_buf[PAGE_SIZE * 2];
u8_t ConfigManager::spiffs_fds[32 * MAX_CONCURRENT_FDS];
u8_t ConfigManager::spiffs_cache_buf[(PAGE_SIZE + 32) * 4];

spiffs ConfigManager::_fs;

// glue code between SPI driver and filesystem
int ConfigManager::spi_read(unsigned int addr, unsigned int size, unsigned char* data) {
    if (_flash.read(addr, size, (char*) data))
        return SPIFFS_OK;
    return -1;
}
int ConfigManager::spi_write(unsigned int addr, unsigned int size, unsigned char* data) {
    if (_flash.write(addr, size, (const char*) data))
        return SPIFFS_OK;
    return -1;
}

bool ConfigManager::MoveUserFile(const char* file, const char* dest) {
    if(PVDO())
        return false;

    spiffs_DIR dir;
    SPIFFS_opendir(&_fs, user_dir, &dir);

    char filename[32];
    snprintf(filename, 32, "u_%s", file);

    char new_name[32];
    snprintf(new_name, 32, "u_%s", dest);

    bool ret = MoveFile(dir.fs, filename, new_name);

    SPIFFS_closedir(&dir);

    return ret;
}

bool ConfigManager::SaveUserFile(const char* file, void* data, uint32_t size) {
    if(PVDO())
        return false;

    spiffs_DIR dir;
    SPIFFS_opendir(&_fs, user_dir, &dir);

    char filename[32];
    snprintf(filename, 32, "u_%s", file);

    bool ret = SaveFile(dir.fs, filename, data, size);

    SPIFFS_closedir(&dir);

    return ret;

}

bool ConfigManager::ReadUserFile(const char* file, void* data, uint32_t size) {
    if(PVDO())
        return false;

    spiffs_DIR dir;
    SPIFFS_opendir(&_fs, user_dir, &dir);

    char filename[32];
    snprintf(filename, 32, "u_%s", file);

    bool ret = ReadFile(dir.fs, filename, data, size);

    SPIFFS_closedir(&dir);

    return ret;
}

bool ConfigManager::DeleteUserFile(const char* file) {
    if(PVDO())
        return false;

    spiffs_DIR dir;
    SPIFFS_opendir(&_fs, user_dir, &dir);

    char filename[32];
    snprintf(filename, 32, "u_%s", file);

    mutex.lock();
    int handle = SPIFFS_remove(dir.fs, filename);
    mutex.unlock();

    if (handle < 0) {
        printf("SPIFFS_remove failed %d", SPIFFS_errno(dir.fs));
    }

    SPIFFS_closedir(&dir);
    return handle == SPIFFS_OK;
}

bool ConfigManager::DeleteFile(const char* file) {
    if(PVDO())
        return false;

    spiffs_DIR dir;
    SPIFFS_opendir(&_fs, user_dir, &dir);

    mutex.lock();
    int handle = SPIFFS_remove(dir.fs, file);
    mutex.unlock();

    SPIFFS_closedir(&dir);
    return handle == SPIFFS_OK;
}

uint32_t ConfigManager::UsedSpace() {

    if(PVDO())
        return 0;

    spiffs_DIR dir;
    SPIFFS_opendir(&_fs, user_dir, &dir);

    spiffs_dirent entry;

    uint32_t used_space = 0;
    while (SPIFFS_readdir(&dir, &entry) != NULL) {
        used_space += entry.size;
    }

    SPIFFS_closedir(&dir);
    return used_space;
}

bool ConfigManager::AppendUserFile(const char* file, void* data, uint32_t size) {
    if(PVDO())
        return false;

    spiffs_DIR dir;
    SPIFFS_opendir(&_fs, user_dir, &dir);

    char filename[32];
    snprintf(filename, 32, "u_%s", file);

    bool ret = AppendFile(dir.fs, filename, data, size);

    SPIFFS_closedir(&dir);

    return ret;
}

file_record ConfigManager::OpenUserFile(const char* file, int mode) {
    file_record mf;
    if(PVDO()){
        mf.fd = -1;
        return mf;
    }
    if (strlen(file) > 30) {
        mf.fd = -1;
    } else {
        char filename[32];
        snprintf(filename, 32, "u_%s", file);

        mf = OpenFile(&_fs, filename, mode);

        memset(mf.name, 0, 30);
        snprintf(mf.name, 31, "%s", file);
    }

    return mf;
}

bool ConfigManager::SeekUserFile(file_record& file, size_t offset, int whence) {
    return SeekFile(&_fs, file, offset, whence);
}

int ConfigManager::ReadUserFile(file_record& file, void* data, size_t length) {
    return ReadFile(&_fs, file, data, length);
}

int ConfigManager::WriteUserFile(file_record& file, void* data, size_t length) {
    return WriteFile(&_fs, file, data, length);
}

bool ConfigManager::CloseUserFile(file_record& file) {
    return CloseFile(&_fs, file);
}

file_record ConfigManager::OpenFile(spiffs* fs, const char* file, int mode) {
    file_record mf;
    if(PVDO()){
        mf.fd = -1;
        return mf;
    }

    mutex.lock();

    if (_openFds >= MAX_CONCURRENT_FDS - 1) {
        printf("Open file descriptors at max %d", _openFds);
        mf.fd = -1;
    } else {
        mf.fd = SPIFFS_open(&_fs, file, mode, 0);

        if (mf.fd < 0) {
            printf("SPIFFS_open failed %d", SPIFFS_errno(&_fs));
            SPIFFS_close(&_fs, mf.fd);
            mf.fd = -1;
        } else {
            snprintf(mf.name, 30, file);

            spiffs_stat stat;
            if (SPIFFS_stat(&_fs, file, &stat) != SPIFFS_OK) {
                printf("SPIFFS_stat failed %d", SPIFFS_errno(&_fs));
                SPIFFS_close(&_fs, mf.fd);
                mf.fd = -1;
            } else {
                _openFds++;
                mf.size = stat.size;
            }
        }
        mutex.unlock();
    }
    return mf;
}

bool ConfigManager::SeekFile(spiffs* fs, file_record& file, size_t offset, int whence) {
    if(PVDO())
        return false;

    mutex.lock();

    if (SPIFFS_lseek(fs, file.fd, offset, whence) != SPIFFS_OK) {
        printf("SPIFFS_lseek failed %d", SPIFFS_errno(&_fs));
        mutex.unlock();
        return false;
    }
    mutex.unlock();
    return true;
}

int ConfigManager::MoveFile(spiffs* fs, file_record& file, const char* new_name) {
    if(PVDO())
        return -1;

    mutex.lock();
    int ret = SPIFFS_move(fs, file.name, new_name);
    mutex.unlock();
    return ret;
}

int ConfigManager::ReadFile(spiffs* fs, file_record& file, void* data, size_t length) {
    if(PVDO())
        return 0;
    mutex.lock();
    int ret = SPIFFS_read(fs, file.fd, data, length);
    mutex.unlock();
    return ret;
}

int ConfigManager::WriteFile(spiffs* fs, file_record& file, void* data, size_t length) {
    if(PVDO())
        return 0;
    mutex.lock();
    int ret = SPIFFS_write(fs, file.fd, data, length);
    mutex.unlock();
    return ret;
}

bool ConfigManager::CloseFile(spiffs* fs, file_record& file) {
    if(PVDO())
        return false;
    mutex.lock();
    SPIFFS_close(fs, file.fd);
    mutex.unlock();
    file.fd = 0;
    _openFds--;
    return true;
}
#endif /* TARGET_MTS_MDOT_F411RE */

ConfigManager::~ConfigManager() {
#if defined (TARGET_MTS_MDOT_F411RE)
    if (! PVDO()) {
        mutex.lock();
        SPIFFS_unmount(&_fs);
        mutex.unlock();
    }
#endif /* TARGET_MTS_MDOT_F411RE */
}

#if defined (TARGET_MTS_MDOT_F411RE)
bool ConfigManager::MoveFile(spiffs* fs, const char* file, const char* new_name) {
    if(PVDO())
        return false;
    mutex.lock();
    bool ret = (SPIFFS_move(fs, file, new_name) == SPIFFS_OK);
    mutex.unlock();
    return ret;
}

bool ConfigManager::MoveUserFile(file_record& file, const char* new_name) {
    if(PVDO())
        return false;

    return MoveUserFile(file.name, new_name);
}

bool ConfigManager::MoveUserFileToFirwareUpgrade(const char* file) {
    char filename[32];
    snprintf(filename, 32, "u_%s", file);

    return MoveFile(&_fs, filename, "fw_upgrade.bin");
}
#endif /* TARGET_MTS_MDOT_F411RE */


#if defined (TARGET_MTS_MDOT_F411RE)
int ConfigManager::spi_erase(unsigned int addr, unsigned int size) {
    mutex.lock();
    _flash.clear_sector(addr);
    mutex.unlock();
    return SPIFFS_OK;
}
#endif /* TARGET_MTS_MDOT_F411RE */

ConfigManager::ConfigManager()
{
#if defined (TARGET_MTS_MDOT_F411RE)
    EnablePVD();
#endif /* TARGET_MTS_MDOT_F411RE */
    Wakeup();
    Mount();
}

void ConfigManager::Sleep() {
#if defined (TARGET_MTS_MDOT_F411RE)
    _flash.deep_power_down();
#endif /* TARGET_MTS_MDOT_F411RE */
}

void ConfigManager::Wakeup() {
#if defined (TARGET_MTS_MDOT_F411RE)
    _flash.wakeup();
#endif /* TARGET_MTS_MDOT_F411RE */
}

#if defined (TARGET_MTS_MDOT_F411RE)
void ConfigManager::EnablePVD(){
    PWR->CR &= ~PWR_CR_PLS;
    PWR->CR |= PWR_CR_PLS_LEV4;
    PWR->CR |= PWR_CR_PVDE;
}
bool ConfigManager::PVDO(){
    // Explicitly bitmask to a temp variable since PWR->CSR has some read only bits
    uint32_t pvdo = PWR->CSR & PWR_CSR_PVDO;
    if (pvdo) {
        printf("Cannot access serial flash. Voltage too low!");
        return true;
    }
    return false;
}
#endif /* TARGET_MTS_MDOT_F411RE */

void ConfigManager::Mount() {
#if defined (TARGET_MTS_MDOT_F411RE)
    if(PVDO())
        return;
    spiffs_config cfg;
    // configure the filesystem
    cfg.phys_size = MEM_SIZE;
    cfg.phys_addr = 0;
    cfg.phys_erase_block = SECTOR_SIZE;
    cfg.log_block_size = SECTOR_SIZE;
    cfg.log_page_size = PAGE_SIZE;

    cfg.hal_read_f = &spi_read;
    cfg.hal_write_f = &spi_write;
    cfg.hal_erase_f = &spi_erase;

    // mount the filesystem
    mutex.lock();
    int ret = SPIFFS_mount(&_fs, &cfg, spiffs_work_buf, spiffs_fds, sizeof(spiffs_fds), spiffs_cache_buf,
                           sizeof(spiffs_cache_buf),
                           NULL);
    mutex.unlock();
    if (ret) {
        printf("SPIFFS_mount failed %d - can't continue", ret);
    }

    _openFds = 0;
#endif /* TARGET_MTS_MDOT_F411RE */
}

bool ConfigManager::SaveSession(NetworkSession_t& s) {
    ScopedRomWriteLock make_rom_writable;
    bool ret;

#if defined (TARGET_MTS_MDOT_F411RE)
    while (DeleteFile(session_file)) {
        printf("Removed old session file");
    }
    ret = SaveFile(&_fs, session_file, &s, sizeof(s));
#else
    if (xdot_eeprom_write_buf(SESSION_ADDR, (uint8_t*)&s, sizeof(s)))
        ret = false;
    else
        ret = true;
#endif /* TARGET_MTS_MDOT_F411RE */
    return ret;
}

bool ConfigManager::SaveProtected(ProtectedSettings_t& p) {
    ScopedRomWriteLock make_rom_writable;
    bool ret;
#if defined (TARGET_MTS_MDOT_F411RE)
    ret = SaveFile(&_fs, protected_file, &p, sizeof(p));
#else
    if (xdot_eeprom_write_buf(PROTECTED_ADDR, (uint8_t*)&p, sizeof(p)))
        ret = false;
    else
        ret = true;
#endif /* TARGET_MTS_MDOT_F411RE */
    return ret;
}

bool ConfigManager::Save(NetworkSettings_t& n) {
    bool ret;
    ScopedRomWriteLock make_rom_writable;

#if defined (TARGET_MTS_MDOT_F411RE)
    ret = SaveFile(&_fs, file, &n, sizeof(n));
#else
    if (xdot_eeprom_write_buf(SETTINGS_ADDR, (uint8_t*)&n, sizeof(n)))
        ret = false;
    else
        ret = true;
#endif /* TARGET_MTS_MDOT_F411RE */

    return ret;
}


bool ConfigManager::SaveSettings(ApplicationSettings_t& a) {
    bool ret;
    ScopedRomWriteLock make_rom_writable;

#if defined (TARGET_MTS_MDOT_F411RE)
    ret = SaveFile(&_fs, app_settings_file, &a, sizeof(a));
#else
    if (xdot_eeprom_write_buf(USER_ADDR, (uint8_t*)&a, sizeof(a)))
        ret = false;
    else
        ret = true;
#endif /* TARGET_MTS_MDOT_F411RE */

    return ret;
}

#if defined (TARGET_MTS_MDOT_F411RE)
bool ConfigManager::AppendFile(spiffs *fs, const char* file, void* data, uint32_t size) {
    if(PVDO())
        return false;

    // write to the file
    int ret;
    mutex.lock();
    int handle = SPIFFS_open(fs, file, SPIFFS_CREAT | SPIFFS_RDWR | SPIFFS_APPEND, 0);
    if (handle < 0) {
        printf("SPIFFS_open failed %d", SPIFFS_errno(fs));
        mutex.unlock();
        return false;
    }

    if (handle) {
        ret = SPIFFS_write(fs, handle, data, size);
        if (ret < 0)
            printf("SPIFFS_write failed %d", SPIFFS_errno(fs));

        SPIFFS_close(fs, handle);
    }

    // read the current file contents
    spiffs_stat stat;
    memset(&stat, 0, sizeof(stat));
    ret = SPIFFS_stat(fs, file, &stat);
    if (ret)
        printf("SPIFFS_stat failed %d", SPIFFS_errno(fs));

    SPIFFS_close(fs, handle);
    mutex.unlock();
    return true;
}

bool ConfigManager::SaveFile(spiffs *fs, const char* file, void* data, uint32_t size) {
    if(PVDO())
        return false;

    mutex.lock();
    // See case 5076531 and bug 5076213.
    // Read an existing file (protected config) before saving to make sure the FFS is alive and well so we don't create duplicate session files.
    // Also delete any and all session files in case the 'existing file' read does not prevent duplicates.
    if(strcmp(file, session_file) == 0) {
        int retries = 10;
        int handle;
        spiffs_stat stat;
        memset(&stat, 0, sizeof(stat));

        while(retries-- > 0) {
           // read protected config file
            handle = SPIFFS_stat(fs, protected_file, &stat);
            if (!handle) {
                retries = 0;
                printf("Read protected file");
            }
            else {
                wait_ms(100);
                printf("Unable to read protected file... try again");
            }
        }

        retries = 10;
        do {
            handle = SPIFFS_remove(fs, file);
            if((retries == 10) && (handle >= 0)) {
                printf("Removed file: %s", file);
            }
            if ((retries < 10) && (handle >= 0)) {
                printf("Deleted duplicate session file: %s", file);
            }
        } while ((handle >= 0) && (retries-- > 0));
    }

    // write to the file
    int ret;
    int handle = SPIFFS_open(fs, file, SPIFFS_CREAT | SPIFFS_RDWR | SPIFFS_TRUNC, 0);
    if (handle < 0) {
        printf("SPIFFS_open failed %d", SPIFFS_errno(fs));
        mutex.unlock();
        return false;
    }

    if (handle) {
        ret = SPIFFS_write(fs, handle, data, size);
        if (ret < 0)
            printf("SPIFFS_write failed %d", SPIFFS_errno(fs));

        SPIFFS_close(fs, handle);
    }

    // read the current file contents
    spiffs_stat stat;
    memset(&stat, 0, sizeof(stat));
    ret = SPIFFS_stat(fs, file, &stat);
    if (ret)
        printf("SPIFFS_stat failed %d", SPIFFS_errno(fs));

    SPIFFS_close(fs, handle);
    mutex.unlock();
    return true;
}

bool ConfigManager::ReadFile(spiffs *fs, const char* file, void* dest, uint32_t size) {
    if(PVDO())
        return false;

    // read the current file contents
    spiffs_stat stat;
    memset(&stat, 0, sizeof(stat));
    mutex.lock();
    int ret = SPIFFS_stat(fs, file, &stat);
    if (ret) {
        printf( "Failed to file in flash.");
        mutex.unlock();
        return false;
    }
    else if (stat.size != size) {
        printf( "File from flash wrong size. Expected %lu - Actual %lu", size, stat.size);
        mutex.unlock();
        return false;
    }

    int handle = SPIFFS_open(fs, file, SPIFFS_RDWR, 0);
    if (handle < 0) {
        printf("SPIFFS_open failed %d", SPIFFS_errno(fs));
        mutex.unlock();
        return false;
    }

    if (handle) {
        uint32_t bytes_read = 0;
        while (bytes_read < stat.size) {
            uint32_t bytes_left = ((stat.size - bytes_read) < size ? (stat.size - bytes_read) : size);

            ret = SPIFFS_read(fs, handle, dest, bytes_left);
            if (ret < 0) {
                printf("SPIFFS_read failed %d", SPIFFS_errno(fs));
                mutex.unlock();
                return false;
            }

            bytes_read += ret;
        }

        SPIFFS_close(&_fs, handle);
    }
    mutex.unlock();
    return true;
}
#endif /* TARGET_MTS_MDOT_F411RE */

void ConfigManager::Load(DeviceConfig_t& dc) {

    // Need to load protected settings first so we can use as defaults for main cfg
#if defined (TARGET_MTS_MDOT_F411RE)
    if (!ReadFile(&_fs, protected_file, &dc.provisioning, sizeof(dc.provisioning))) {
#else
    if (xdot_eeprom_read_buf(PROTECTED_ADDR, (uint8_t*)&dc.provisioning, sizeof(dc.provisioning))) {
        printf("Failed to read protected configuration from EEPROM.");
#endif /* TARGET_MTS_MDOT_F411RE */
        printf("Defaulting protected settings.");
        DefaultProtected(dc);
    }

#if defined (TARGET_MTS_MDOT_F411RE)
    if (!ReadFile(&_fs, file, &dc.settings, sizeof(dc.settings))) {
#else
    if (xdot_eeprom_read_buf(SETTINGS_ADDR, (uint8_t*)&dc.settings, sizeof(dc.settings))) {
        printf("Failed to read configuration from EEPROM.");
#endif /* TARGET_MTS_MDOT_F411RE */
        printf("Net Settings to defaults.");
        Default(dc);
    }

#if defined (TARGET_MTS_MDOT_F411RE)
    if (!ReadFile(&_fs, session_file, &dc.session, sizeof(dc.session))) {
#else
    if (xdot_eeprom_read_buf(SESSION_ADDR, (uint8_t*)&dc.session, sizeof(dc.session))) {
        printf("Failed to read session from EEPROM.");
#endif /* TARGET_MTS_MDOT_F411RE */
        printf("Session to defaults.");
        DefaultSession(dc);
    }

#if defined (TARGET_MTS_MDOT_F411RE)
    if (!ReadFile(&_fs, session_file, &dc.app_settings, sizeof(dc.app_settings))) {
#else
    if (xdot_eeprom_read_buf(USER_ADDR, (uint8_t*)&dc.app_settings, sizeof(dc.app_settings))) {
        printf("Failed to read session from EEPROM.");
#endif /* TARGET_MTS_MDOT_F411RE */
        printf("App Settings to defaults.");
        DefaultSettings(dc);
    }



}

void ConfigManager::DefaultSettings(DeviceConfig_t& dc) {
    dc.app_settings.DutyCycleEnabled = MBED_CONF_LORA_DUTY_CYCLE_ON;
    dc.app_settings.TxInterval = 10000;
}

void ConfigManager::DefaultSession(DeviceConfig_t& dc) {

}

void ConfigManager::DefaultProtected(DeviceConfig_t& dc) {

}

void ConfigManager::Default(DeviceConfig_t& dc) {

}

