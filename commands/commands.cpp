/**********************************************************************
* COPYRIGHT 2015 MULTI-TECH SYSTEMS, INC.
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

#include "commands.h"
#include "lorawan_types.h"

extern Serial pc;
extern DeviceConfig_t device_config;
extern ConfigManager config_mng;

bool exit_cmd_mode = false;

static char prompt[] = "$ ";
static char ok_str[] = "\r\nOK\r\n";
static char error_str[] = "\r\nerror\r\n";
static char invalid_args_str[] = "\r\ninvalid args\r\n";

tinysh_cmd_t reset_cmd = { 0, "reset", "reset command", "", reset_func, 0, 0, 0 };
tinysh_cmd_t run_cmd = { 0, "run", "run command", "", run_func, 0, 0, 0 };
tinysh_cmd_t deveui_cmd = { 0, "deveui", "deveui command", "", deveui_func, 0, 0, 0 };
tinysh_cmd_t appeui_cmd = { 0, "appeui", "appeui command", "", appeui_func, 0, 0, 0 };
tinysh_cmd_t appkey_cmd = { 0, "appkey", "appkey command", "", appkey_func, 0, 0, 0 };
tinysh_cmd_t savep_cmd = { 0, "savep", "save provisioning", "", savep_func, 0, 0, 0 };
tinysh_cmd_t save_cmd = { 0, "save", "save settings", "", save_func, 0, 0, 0 };
tinysh_cmd_t ack_retries_cmd = { 0, "retries", "ack retries setting", "0:disable,1-8", ack_retries_func, 0, 0, 0 };
tinysh_cmd_t datarate_cmd = { 0, "datarate", "datarate setting", "0-15", datarate_func, 0, 0, 0 };
tinysh_cmd_t device_class_cmd = { 0, "class", "device class setting", "A or C", device_class_func, 0, 0, 0 };
tinysh_cmd_t adr_cmd = { 0, "adr", "adr enabled", "0:disabled, 1:enabled", adr_func, 0, 0, 0 };
tinysh_cmd_t duty_cycle_cmd = { 0, "dutycycle", "Duty Cycle enabled", "0:disabled, 1:enabled", duty_cycle_func, 0, 0, 0 };
tinysh_cmd_t tx_interval_cmd = { 0, "txinterval", "Tx interval", "Timeout in ms", tx_interval_func, 0, 0, 0 };
tinysh_cmd_t app_port_cmd = { 0, "port", "Application port", "0-255", app_port_func, 0, 0, 0 };

void reset_func(int argc, char **argv) {
    HAL_NVIC_SystemReset();
}

void run_func(int argc, char **argv) {
    exit_cmd_mode = true;
}

int read_hex_str(const char* input, uint8_t* val, size_t length) {
    unsigned char tmp = 0;
    for (size_t i = 0, j = 0; i < length; ++i, j+=2) {
        if (sscanf(input+j, "%2hhx", &tmp)) {
            val[i] = tmp;
        } else {
            return -1;
        }
    }
    return 0;
}

void print_hex_str(const uint8_t* val, size_t length) {
    printf("\r\n");
    for (size_t i = 0; i < length; ++i) {
        printf("%02x", val[i]);
    }
    printf("\r\n");
}

void deveui_func(int argc, char **argv) {
    if (argc == 1) {
        print_hex_str(device_config.provisioning.DeviceEUI, 8);
        printf("\r\n");
    } else if (argc == 2 && (strlen(argv[1]) == 16)) {
        read_hex_str(argv[1], device_config.provisioning.DeviceEUI, 8);
        printf(ok_str);
    } else {
        printf(invalid_args_str);
    }
}

void appeui_func(int argc, char **argv) {
    if (argc == 1) {
        print_hex_str(device_config.settings.AppEUI, 8);
        printf("\r\n");
    } else if (argc == 2 && (strlen(argv[1]) == 16)) {
        read_hex_str(argv[1], device_config.settings.AppEUI, 8);
        printf(ok_str);
    } else {
        printf(invalid_args_str);
    }
}

void appkey_func(int argc, char **argv) {
    if (argc == 1) {
        print_hex_str(device_config.settings.AppKey, 16);
        printf("\r\n");
    } else if (argc == 2 && (strlen(argv[1]) == 32)) {
        read_hex_str(argv[1], device_config.settings.AppKey, 16);
        printf(ok_str);
    } else {
        printf(invalid_args_str);
    }
}

void ack_retries_func(int argc, char **argv) {
    if (argc == 1) {
        printf("\r\n%u\r\n", device_config.settings.ACKAttempts);
    } else if (argc == 2 && (strlen(argv[1]) == 1)) {
        read_hex_str(argv[1], &device_config.settings.ACKAttempts, 1);
        printf(ok_str);
    } else {
        printf(invalid_args_str);
    }
}

void datarate_func(int argc, char **argv) {
    if (argc == 1) {
        printf("\r\nDR%u\r\n", device_config.settings.TxDataRate);
    } else if (argc == 2 && (strlen(argv[1]) == 1)) {
        read_hex_str(argv[1], &device_config.settings.TxDataRate, 1);
        printf(ok_str);
    } else {
        printf(invalid_args_str);
    }
}

void device_class_func(int argc, char **argv) {
    if (argc == 1) {
        printf("\r\n%s\r\n", device_config.settings.Class == CLASS_A ? "A" : "C");
    } else if (argc == 2 && strlen(argv[1]) == 1) {
        if (argv[1][0] == 'A' || argv[1][0] == 'a') {
            device_config.settings.Class = CLASS_A;
            printf(ok_str);
        } else if (argv[1][0] == 'C' || argv[1][0] == 'c') {
            device_config.settings.Class = CLASS_C;
            printf(ok_str);
        } else {
            printf(invalid_args_str);
        }
    } else {
        printf(invalid_args_str);
    }
}

void adr_func(int argc, char **argv) {
    if (argc == 1) {
        printf("\r\n%d\r\n", device_config.settings.EnableADR);
    } else if (argc == 2 && strlen(argv[1]) == 1) {
        if (argv[1][0] == '0' || argv[1][0] == '1') {
            device_config.settings.EnableADR = (argv[1][0] == '1');
            printf(ok_str);
        } else {
            printf(invalid_args_str);
        }
    } else {
        printf(invalid_args_str);
    }
}

void app_port_func(int argc, char **argv) {
    if (argc == 1) {
        printf("\r\n%d\r\n", device_config.settings.Port);
    } else if (argc == 2) {
        int val = 1;
        if (sscanf(argv[1], "%d", &val)) {
            device_config.settings.Port = val;
            printf(ok_str);
        } else {
            printf(invalid_args_str);
        }
    } else {
        printf(invalid_args_str);
    }
}

void duty_cycle_func(int argc, char **argv) {
    if (argc == 1) {
        printf("\r\n%d\r\n", device_config.app_settings.DutyCycleEnabled);
    } else if (argc == 2 && strlen(argv[1]) == 1) {
        if (argv[1][0] == '0' || argv[1][0] == '1') {
            device_config.app_settings.DutyCycleEnabled = (argv[1][0] == '1');
            printf(ok_str);
        } else {
            printf(invalid_args_str);
        }
    } else {
        printf(invalid_args_str);
    }
}

void tx_interval_func(int argc, char **argv) {
    if (argc == 1) {
        printf("\r\n%lu\r\n", device_config.app_settings.TxInterval);
    } else if (argc == 2) {
        int val = 10000;
        if (sscanf(argv[1], "%d", &val)) {
            device_config.app_settings.TxInterval = val;
            printf(ok_str);
        } else {
            printf(invalid_args_str);
        }
    } else {
        printf(invalid_args_str);
    }
}

void savep_func(int argc, char **argv) {
    if (argc == 1) {
        if (config_mng.SaveProtected(device_config.provisioning)) {
            printf(ok_str);
        } else {
            printf(error_str);
        }
    } else {
        printf(invalid_args_str);
    }
}

void save_func(int argc, char **argv) {
    if (argc == 1) {
        if (config_mng.Save(device_config.settings)
            && config_mng.SaveSettings(device_config.app_settings)) {
            printf(ok_str);
        } else {
            printf(error_str);
        }
    } else {
        printf(invalid_args_str);
    }
}

void tinysh_char_out(unsigned char c) {
    pc.putc(c);
}

void tinyshell_thread() {
    pc.printf("MTS LoRaWAN shell build %s %s\r\n", __DATE__, __TIME__);

    tinysh_set_prompt(prompt);
    tinysh_add_command(&reset_cmd);
    tinysh_add_command(&run_cmd);
    tinysh_add_command(&deveui_cmd);
    tinysh_add_command(&appeui_cmd);
    tinysh_add_command(&appkey_cmd);
    tinysh_add_command(&ack_retries_cmd);
    tinysh_add_command(&datarate_cmd);
    tinysh_add_command(&device_class_cmd);
    tinysh_add_command(&adr_cmd);
    tinysh_add_command(&app_port_cmd);
    tinysh_add_command(&tx_interval_cmd);
    tinysh_add_command(&duty_cycle_cmd);
    tinysh_add_command(&savep_cmd);
    tinysh_add_command(&save_cmd);

    while (!exit_cmd_mode) {
        tinysh_char_in(pc.getc());
    }

}




