#include <kern/ipc/pubsub.h>
#include <lib/string.h> 
// Initialize the message queue
void init_message_queue(struct Topic *topic) {
    topic->head = 0;
    topic->tail = 0;
    topic->count = 0;
}

// Add a message to the queue
void enqueue_message(struct Topic *topic, const char *data) {
    if (topic->count == MAX_QUEUE_SIZE) {
        // Discard the oldest message
        topic->head = (topic->head + 1) % MAX_QUEUE_SIZE;
        topic->count--;
    }

    // Add the new message
    strncpy(topic->queue[topic->tail].data, data, MESSAGE_SIZE);
    topic->tail = (topic->tail + 1) % MAX_QUEUE_SIZE;
    topic->count++;
}

// Remove a message from the queue
void dequeue_message(struct Topic *topic) {
    if (topic->count > 0) {
        topic->head = (topic->head + 1) % MAX_QUEUE_SIZE;
        topic->count--;
    }
}