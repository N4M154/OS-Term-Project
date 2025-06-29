#include <kern/ipc/pubsub.h>

// Subscribe to a topic
void subscribe(const char *topic_name, void (*callback)(struct Message *)) {
    struct Topic *topic = find_topic(topic_name);
    if (topic == NULL) {
        // Create the topic if it doesn't exist
        topic = create_topic(topic_name);
        if (topic == NULL) {
            return; // Failed to create topic
        }
    }

    // Add the subscriber to the list
    if (topic->num_subscribers >= MAX_SUBSCRIBERS) {
        return; // Too many subscribers
    }

    topic->subscribers[topic->num_subscribers++].callback = callback;
}