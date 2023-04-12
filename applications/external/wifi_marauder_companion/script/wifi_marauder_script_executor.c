#include "../wifi_marauder_app_i.h"
#include "wifi_marauder_script_executor.h"

void _wifi_marauder_script_delay(WifiMarauderScriptWorker* worker, uint32_t delay_secs) {
    for(uint32_t i = 0; i < delay_secs && worker->is_running; i++) furi_delay_ms(1000);
}

void _send_stop() {
    const char stop_command[] = "stopscan\n";
    wifi_marauder_uart_tx((uint8_t*)(stop_command), strlen(stop_command));
}

void _send_line_break() {
    wifi_marauder_uart_tx((uint8_t*)("\n"), 1);
}

void _wifi_marauder_script_execute_scan(
    WifiMarauderScriptStageScan* stage,
    WifiMarauderScriptWorker* worker) {
    char command[10];
    if(stage->type == WifiMarauderScriptScanTypeAp) {
        snprintf(command, sizeof(command), "scanap\n");
    } else {
        snprintf(command, sizeof(command), "scansta\n");
    }
    wifi_marauder_uart_tx((uint8_t*)(command), strlen(command));
    _wifi_marauder_script_delay(worker, stage->timeout);
    _send_stop();
}

void _wifi_marauder_script_execute_select(WifiMarauderScriptStageSelect* stage) {
    const char* select_type = NULL;
    switch(stage->type) {
    case WifiMarauderScriptSelectTypeAp:
        select_type = "-a";
        break;
    case WifiMarauderScriptSelectTypeStation:
        select_type = "-c";
        break;
    case WifiMarauderScriptSelectTypeSsid:
        select_type = "-s";
        break;
    default:
        return; // invalid stage
    }

    char command[256];
    if(strcmp(stage->filter, "all") == 0) {
        snprintf(command, sizeof(command), "select %s all\n", select_type);
    } else {
        snprintf(command, sizeof(command), "select %s {%s}\n", select_type, stage->filter);
    }

    const size_t command_length = strlen(command);
    wifi_marauder_uart_tx((uint8_t*)command, command_length);
}

void _wifi_marauder_script_execute_beacon_list(
    WifiMarauderScriptStageBeaconList* stage,
    WifiMarauderScriptWorker* worker) {
    char command[100];
    char* ssid;
    for(int i = 0; i < stage->ssid_count; i++) {
        ssid = stage->ssids[i];
        snprintf(command, sizeof(command), "ssid -a -n \"%s\"", ssid);
        wifi_marauder_uart_tx((uint8_t*)(command), strlen(command));
        _send_line_break();
    }
    const char attack_command[] = "attack -t beacon -l\n";
    wifi_marauder_uart_tx((uint8_t*)(attack_command), strlen(attack_command));
    _wifi_marauder_script_delay(worker, stage->timeout);
    _send_stop();
}

void wifi_marauder_script_execute_stage(WifiMarauderScriptStage* stage, void* context) {
    furi_assert(context);
    WifiMarauderScriptWorker* worker = context;
    void* stage_data = stage->stage;

    switch(stage->type) {
    case WifiMarauderScriptStageTypeScan:
        _wifi_marauder_script_execute_scan((WifiMarauderScriptStageScan*)stage_data, worker);
        break;
    case WifiMarauderScriptStageTypeSelect:
        _wifi_marauder_script_execute_select((WifiMarauderScriptStageSelect*)stage_data);
        break;
    case WifiMarauderScriptStageTypeBeaconList:
        _wifi_marauder_script_execute_beacon_list(
            (WifiMarauderScriptStageBeaconList*)stage_data, worker);
        break;
    default:
        break;
    }
}