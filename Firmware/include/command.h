#pragma once

#include <Arduino.h>
#include <AsyncTCP.h>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "util.h"

#define OTGW_MAX_CLIENTS 4
#define OTGW_MUTEX_TIMEOUT_MS 100

extern class OtGwCommand {
private:
    bool enableOtEvents;
    AsyncServer server;
    std::vector<AsyncClient*> clients;
    SemaphoreHandle_t clientsMutex;
    friend void handleNewClient(void* arg, AsyncClient* client);
    friend void handleClientData(void* arg, AsyncClient* client, void *data, size_t len);
    friend void handleClientDisconnect(void* arg, AsyncClient* client);
    void onNewClient(void* arg, AsyncClient* client);
    void onClientData(void* arg, AsyncClient* client, void *data, size_t len);
    void onClientDisconnect(void* arg, AsyncClient* client);
public:
    OtGwCommand();
    void begin();
    void loop();
    void sendAll(const String &s);
    void sendOtEvent(const char source, const uint32_t data);
    
} command;

