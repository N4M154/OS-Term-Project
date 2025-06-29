#include <kern/ipc/pubsub.h>
#include <lib/string.h>
#include <kern/ipc/topic.c>
#include <kern/ipc/queue.c>

// Publish a message to a topic
void publish(const char *topic_name, const char *message_data) {
    struct Topic *topic = find_topic(topic_name);
    if (topic == NULL) {
        // Create the topic if it doesn't exist
        topic = create_topic(topic_name);
        if (topic == NULL) {
            return; // Failed to create topic
        }
    }

    // Add the message to the queue
    enqueue_message(topic, message_data);

    // Notify all subscribers
    for (int i = 0; i < topic->num_subscribers; i++) {
        topic->subscribers[i].callback(&topic->queue[topic->head]);
    }
}
