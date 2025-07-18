#ifndef COMM_INTERFACE
#define COMM_INTERFACE

typedef struct {
    void (*Init)(void);
    void (*Send)(uint8_t* data, uint16_t len);
    void (*Receive)(uint8_t* buffer, uint16_t len);
} CommunicationInterface;


#endif
