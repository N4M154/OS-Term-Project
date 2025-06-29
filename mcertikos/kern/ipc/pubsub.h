#ifndef KERN_IPC_PUBSUB_H
#define KERN_IPC_PUBSUB_H

#include <lib/types.h>

#define MAX_TOPICS 10
#define MAX_SUBSCRIBERS 100
#define MAX_QUEUE_SIZE 1000
#define TOPIC_NAME_LEN 32
#define MESSAGE_SIZE 256

// Message structure
struct Message {
    char data[MESSAGE_SIZE];
};

// Subscriber structure
struct Subscriber {
    void (*callback)(struct Message *msg); // Callback function for message delivery
};

// Topic structure
struct Topic {
    char name[TOPIC_NAME_LEN];             // Topic name
    struct Message queue[MAX_QUEUE_SIZE];  // Message queue
    int head;                              // Index of the oldest message
    int tail;                              // Index of the next free slot
    int count;                             // Number of messages in the queue
    struct Subscriber subscribers[MAX_SUBSCRIBERS]; // List of subscribers
    int num_subscribers;                   // Number of subscribers
};

// Function declarations
struct Topic *find_topic(const char *name);
struct Topic *create_topic(const char *name);
void enqueue_message(struct Topic *topic, const char *data);
void publish(const char *topic_name, const char *message_data);
void subscribe(const char *topic_name, void (*callback)(struct Message *));

#endif // KERN_IPC_PUBSUB_H