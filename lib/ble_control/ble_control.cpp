#include "ble_control.h"

// Service / Characteristic 的 UUID，隨便產生的固定值，手機端程式要連線
// 時也要用同一組 UUID 去找。沒有特殊意義，只要跟其他裝置不要撞名即可。
static const char *SERVICE_UUID = "bba28d1b-f273-489d-9f6c-60bc29a7b526";
static const char *CHARACTERISTIC_UUID = "54e1e117-90d8-4db3-a574-c727a13e43e0";

BleControl::BleControl()
    : _server(nullptr), _characteristic(nullptr), _callback(nullptr),
      _deviceConnected(false), _wasConnected(false) {}

void BleControl::begin(const char *deviceName) {
    BLEDevice::init(deviceName);

    _server = BLEDevice::createServer();
    _server->setCallbacks(this);

    BLEService *service = _server->createService(SERVICE_UUID);

    _characteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_WRITE);
    _characteristic->setCallbacks(this);

    service->start();

    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    BLEDevice::startAdvertising();

    Serial.println("BLE 廣播已啟動");
    Serial.print("裝置名稱: ");
    Serial.println(deviceName);
}

void BleControl::handle() {
    // 斷線後要手動重新開始廣播，不然手機看不到裝置
    if (!_deviceConnected && _wasConnected) {
        BLEDevice::startAdvertising();
        Serial.println("BLE 已斷線，重新開始廣播");
        _wasConnected = false;
    }
}

void BleControl::onButtonPressed(ButtonCallback callback) {
    _callback = callback;
}

void BleControl::onConnect(BLEServer *server) {
    _deviceConnected = true;
    Serial.println("BLE 裝置已連線");
}

void BleControl::onDisconnect(BLEServer *server) {
    _deviceConnected = false;
    _wasConnected = true;
    Serial.println("BLE 裝置已斷線");
}

void BleControl::onWrite(BLECharacteristic *characteristic) {
    std::string value = characteristic->getValue();
    if (value.length() > 0 && _callback != nullptr) {
        _callback(String(value.c_str()));
    }
}
