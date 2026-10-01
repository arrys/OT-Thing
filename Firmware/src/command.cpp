#include "command.h"
#include "portal.h"
#include "otvalues.h"
#include "main.h"

OtGwCommand command;

void handleNewClient(void* arg, AsyncClient* client) {
    command.onNewClient(arg, client);
}

void handleClientData(void* arg, AsyncClient* client, void *data, size_t len) {
    command.onClientData(arg, client, data, len);
}

void handleClientDisconnect(void* arg, AsyncClient* client) {
    command.onClientDisconnect(arg, client);
}

OtGwCommand::OtGwCommand():
        enableOtEvents(true),
        server(25238),
        clientsMutex(xSemaphoreCreateMutex()) {
    server.onClient(&handleNewClient, &server);
}

void OtGwCommand::onNewClient(void* arg, AsyncClient* client) {
    SemHelper lock(clientsMutex, OTGW_MUTEX_TIMEOUT_MS);
    if (!lock) {
        client->close();
        return;
    }

    if (clients.size() >= OTGW_MAX_CLIENTS) {
        client->close();
        return;
    }

    clients.push_back(client);
    client->onData(&handleClientData, NULL);
    client->onDisconnect(&handleClientDisconnect, NULL);
}

void OtGwCommand::onClientData(void* arg, AsyncClient* client, void *data, size_t len) {
    ;
}

void OtGwCommand::onClientDisconnect(void* arg, AsyncClient* client) {
    // Must drop the pointer here: AsyncTCP destroys the AsyncClient once this
    // callback returns, so keeping it in clients would leave sendAll() writing
    // through freed memory and grow the vector without bound.
    SemHelper lock(clientsMutex, OTGW_MUTEX_TIMEOUT_MS);
    if (!lock)
        return;

    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (*it == client) {
            clients.erase(it);
            break;
        }
    }
}

void OtGwCommand::begin() {
    server.begin();
}

void OtGwCommand::sendAll(const String &s) {
    String line(s);
    line += F("\r\n");

    SemHelper lock(clientsMutex, OTGW_MUTEX_TIMEOUT_MS);
    if (lock) {
        for (auto client: clients) {
            if (client->connected())
                client->write(line.c_str());
        }
    }

#ifdef OT_SERIAL
    Serial.print(line);
#endif

#ifdef DEBUG
if (bleClientConnected && bleSerialTx) {
    bleSerialTx->setValue(line.c_str());
    bleSerialTx->notify();
}
#endif
}

void OtGwCommand::sendOtEvent(const char source, const uint32_t data) {
    String line(source);
    int pos = 28;
    while (pos > 0) {
        pos -= 4;
        if (((data>>pos) & 0xF0) != 0)
            break;

        line += '0';
    }
    line += String(data, HEX);

    if (enableOtEvents)
        sendAll(line);

    auto mt = OpenTherm::getMessageType(data);
    auto id = OpenTherm::getDataID(data);

    line += ' ';
    switch (mt) {
    case OpenThermMessageType::READ_DATA:
        line += F("READ");
        break;
    case OpenThermMessageType::WRITE_DATA:
        line += F("WRITE");
        break;
    case OpenThermMessageType::INVALID_DATA:
        line += F("INVALID_DATA");
        break;
    case OpenThermMessageType::READ_ACK:
        line += F("READ_ACK");
        break;
    case OpenThermMessageType::WRITE_ACK:
        line += F("WRITE_ACK");
        break;
    case OpenThermMessageType::DATA_INVALID:
        line += F("DATA_INVALID");
        break;
    case OpenThermMessageType::UNKNOWN_DATA_ID:
        line += F("UKNOWN_ID");
        break;
    default:
        break;
    }

    const char *name = getOTname(id);
    line += ' ';
    if (name != nullptr)
        line += FPSTR(name);
    else {
        line += F("ID ");
        line += String((int) id);
    }
    line += F(" 0x");
    uint16_t mask = 0xF000;
    while (mask > 0x000F) {
        if ((data & mask) == 0)
            line += '0';
        else
            break;
        mask >>= 4;
    }
    line += String(data & 0xFFFF, HEX);
    portal.textAll(line);
}

void OtGwCommand::loop() {
}