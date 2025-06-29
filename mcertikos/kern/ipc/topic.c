#include <lib/string.h>
#include <kern/ipc/pubsub.h>



struct Topic topics[MAX_TOPICS];
int num_topics = 0;

// Find a topic by name
struct Topic *find_topic(const char *name) {
    for (int i = 0; i < num_topics; i++) {
        if (strcmp(topics[i].name, name) == 0) {
            return &topics[i];
        }
    }
    return NULL; // Topic not found
}

// Create a new topic
struct Topic *create_topic(const char *name) {
    if (num_topics >= MAX_TOPICS) {
        return NULL; // No more topics can be created
    }

    struct Topic *topic = &topics[num_topics++];
    strncpy(topic->name, name, TOPIC_NAME_LEN);
    topic->head = 0;
    topic->tail = 0;
    topic->count = 0;
    topic->num_subscribers = 0;
    return topic;
}