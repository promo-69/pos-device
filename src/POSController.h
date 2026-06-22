#ifndef POS_CONTROLLER_H
#define POS_CONTROLLER_H

#include <Arduino.h>

class POSController {
public:
    POSController();
    void init();
    static String tryReadCard(bool fromPayment = false);

private:
    static void taskProcesarCobros(void *parameter);
    static String readCardReal();
    static void executeBankTransaction(const char* operationId);
};

#endif // POS_CONTROLLER_H
