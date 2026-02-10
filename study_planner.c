#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOPICS 100
#define MAX_NAME_LEN 50

// --- Data Structures ---

// Structfor a dependency node (linked list for graph adjacency)
struct Node {
    int topic_id;
    struct Node* next;
};

// Struct for a Topic
struct Topic {
    int id;
    char name[MAX_NAME_LEN];
    int duration; // in hours
    int priority; // Higher value = higher priority (1-10)
    struct Node* dependencies; // List of topics that depend on this one
};

// global Variables
struct Topic Topics[MAX_TOPICS];
int in_degree[MAX_TOPICS];       // Stores how many prerequisites each topic has
int schedule_order[MAX_TOPICS];  // Stores the final calculated order
int order_count = 0;
int total_topics = 0;

// Helper array for the "Ready List" (topics with 0 unsatisfied prerequisites)
int ready[MAX_TOPICS];
int ready_count = 0;

// --- Function Prototypes ---
void add_dependency(int u, int v);
void add_to_ready(int topic_id);
int get_highest_priority_topic();
void generate_priority_order();
void print_schedule(int daily_hours);

int main() {
    int daily_hours;

    // 1. INPUT PHASE
    printf("--- Intelligent Study Planner Input ---\n");
    printf("Enter total number of topics: ");
    scanf("%d", &total_topics);

    for (int i = 0; i < total_topics; i++) {
        Topics[i].id = i;
        Topics[i].dependencies = NULL;
        in_degree[i] = 0;
        
        printf("\nDetails for Topic ID %d:\n", i);
        printf("  Name (no spaces, e.g. Arrays): ");
        scanf("%s", Topics[i].name);
        printf("  Duration (hours): ");
        scanf("%d", &Topics[i].duration);
        printf("  Priority (1-10, 10=Highest): ");
        scanf("%d", &Topics[i].priority);
    }

    int num_deps;
    printf("\nEnter number of dependency rules (e.g. 2 if A->B and B->C): ");
    scanf("%d", &num_deps);

    if (num_deps > 0) {
        printf("Enter dependencies as pairs 'ID1 ID2' (ID1 must be done before ID2):\n");
        for (int i = 0; i < num_deps; i++) {
            int u, v;
            scanf("%d %d", &u, &v);
            if (u >= 0 && u < total_topics && v >= 0 && v < total_topics) {
                add_dependency(u, v);
                // Increase in-degree of the dependent topic (v)
                in_degree[v]++;
            } else {
                printf("  Warning: Invalid IDs %d -> %d ignored.\n", u, v);
            }
        }
    }

    printf("\nEnter max study hours available per day: ");
    scanf("%d", &daily_hours);

    // 2. PROCESSING PHASE
    // This fills the 'schedule_order' array
    generate_priority_order();

    // 3. OUTPUT PHASE
    print_schedule(daily_hours);

    return 0;
}

// --- Implementation ---

// Add edge u -> v (u is prerequisite for v) to Adjacency List
void add_dependency(int u, int v) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->topic_id = v;
    newNode->next = Topics[u].dependencies;
    Topics[u].dependencies = newNode;
}

// Add a topic to the 'ready' list
void add_to_ready(int topic_id) {
    ready[ready_count++] = topic_id;
}

// KEY LOGIC: Find and remove the highest priority topic from 'ready' list
int get_highest_priority_topic() {
    if (ready_count == 0) return -1;

    int best_index = 0;
    int best_topic_id = ready[0];

    // Scan ready list to find the topic with highest priority
    for (int i = 1; i < ready_count; i++) {
        int current_id = ready[i];
        int best_id = ready[best_index]; // ID of currently best found
        
        // Priority comparison
        // If priorities are equal, we stick with the one found first (stable)
        if (Topics[current_id].priority > Topics[best_id].priority) {
            best_index = i;
            best_topic_id = current_id;
        }
    }

    // Remove the selected topic by shifting remaining elements left
    // This is O(N) but fine for mini-projects. A Heap would be O(log N).
    for (int j = best_index; j < ready_count - 1; j++) {
        ready[j] = ready[j + 1];
    }
    ready_count--;

    return best_topic_id;
}

// Core Algorithm: Priority-Based Topological Sort (Modified Kahn's)
void generate_priority_order() {
    order_count = 0;
    ready_count = 0;

    // Step A: Find all topics with 0 prerequisites initially
    for (int i = 0; i < total_topics; i++) {
        if (in_degree[i] == 0) {
            add_to_ready(i);
        }
    }

    // Step B: Process ready list until empty
    while (ready_count > 0) {
        // Always pick the MOST IMPORTANT available topic
        int u = get_highest_priority_topic();
        
        // Add to our final linear schedule
        schedule_order[order_count++] = u;

        // Process neighbors (topics that depend on u)
        struct Node* temp = Topics[u].dependencies;
        while (temp != NULL) {
            int v = temp->topic_id;
            
            // "Remove" the dependency edge
            in_degree[v]--;
            
            // If v has no more prerequisites, it becomes ready
            if (in_degree[v] == 0) {
                add_to_ready(v);
            }
            temp = temp->next;
        }
    }

    // Cycle detection
    if (order_count != total_topics) {
        printf("\nError: Circular dependency detected! (e.g., A depends on B, B depends on A)\n");
        exit(1);
    }
}

// Simple Scheduler to assign ordered topics to days
void print_schedule(int daily_limit) {
    if (order_count == 0) return;

    printf("\n\n=== GENERATED STUDY PLAN ===\n");
    
    int current_day = 1;
    int day_time_used = 0;

    printf("Day %d:\n", current_day);

    for (int i = 0; i < order_count; i++) {
        int topic_id = schedule_order[i];
        int dur = Topics[topic_id].duration;
        char* name = Topics[topic_id].name;
        int prio = Topics[topic_id].priority;

        // Check if we can fit this topic in current day
        if (day_time_used + dur <= daily_limit) {
            printf("  [ ] Study '%s' (%d hrs) - Priority: %d\n", name, dur, prio);
            day_time_used += dur;
        } else {
            // Move to next day
            // If the topic itself is longer than daily limit, we just put it on a new day
            // (Handling huge topics splitting across days is a V2 feature)
            current_day++;
            day_time_used = 0;
            printf("\nDay %d:\n", current_day);
            
            printf("  [ ] Study '%s' (%d hrs) - Priority: %d\n", name, dur, prio);
            day_time_used += dur;
        }
    }
    printf("\n============================\n");
}
