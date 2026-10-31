#ifndef BLE_CONTROL_H
#define BLE_CONTROL_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// 按鍵被按下時會呼叫這個函式，參數是按鍵名稱（例如 "UP"、"BTN1"）
typedef void (*ButtonCallback)(const String &button);

// ESP32 以 BLE Peripheral（週邊）角色廣播，手機等 BLE 主機連上後，
// 把按鍵名稱字串寫入指定的 characteristic 就能觸發 onButtonPressed 回呼。
class BleControl : public BLEServerCallbacks, public BLECharacteristicCallbacks {
public:
    BleControl();

    // 啟動 BLE 並開始廣播，deviceName 是手機掃描時看到的裝置名稱
    void begin(const char *deviceName);

    // 在 loop() 裡持續呼叫，負責在斷線後自動重新開始廣播
    void handle();

    // 設定收到按鍵時要呼叫的函式
    void onButtonPressed(ButtonCallback callback);

private:
    BLEServer *_server;
    BLECharacteristic *_characteristic;
    ButtonCallback _callback;
    bool _deviceConnected;
    bool _wasConnected;

    void onConnect(BLEServer *server) override;
    void onDisconnect(BLEServer *server) override;
    void onWrite(BLECharacteristic *characteristic) override;
};

#endif
