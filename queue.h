#pragma once

template <typename T>
struct PriorityListNode{
    T data;
    uint32_t priority;
    PriorityListNode<T> *next;
};

template <typename T>
PriorityListNode<T> * newPriorityNode(T data, uint32_t priority){
    PriorityListNode<T> *newnode = new PriorityListNode<T>;
    newnode->data = data;
    newnode->next = NULL;
    newnode->priority = priority;
    return(newnode);
}

template <typename T>
void deleteNode(PriorityListNode<T> *node){
    delete(node);
}

template <typename T>
struct PriorityQueue{
    PriorityListNode<T> *front;
    uint32_t size;

    PriorityQueue():front(NULL),size(0){}

    void enqueue(T data, uint32_t priority){
        PriorityListNode<T> *node = newPriorityNode(data, priority);
        if (front){
            node->next = front;
            front = node;
        }
        else{
            front = node;
        }
        size++;
    }

    T dequeue(){
        if (front){
            PriorityListNode<T> *ptr = front, *lowestNode = front, *preptr = NULL, *preLowest = NULL;
            uint32_t lowest = -1;
            // if (front == rear)
            //     rear = NULL;
            while (ptr){
                if(ptr->priority < lowest){
                    lowest = ptr->priority;
                    lowestNode = ptr;
                    preLowest = preptr;
                }
                preptr = ptr;
                ptr = ptr->next;
            }
            if (preLowest) {
                preLowest->next = lowestNode->next;
            }
            else
                front = lowestNode->next;
            T data = lowestNode->data;
            deleteNode(lowestNode);
            size--;
            return(data);
        }
        return(0);
    }

    void updateQueue(T data, uint32_t newPriority){
        PriorityListNode<T> *ptr = front;
        while (ptr){
            if (ptr->data == data){
                ptr->priority = newPriority;
                break;
            }
            ptr = ptr->next;
        }
    }

    bool isEmpty(){
        return(!front);
    }

    void empty() {
        PriorityListNode<T> *ptr = NULL;
        while (front){
            ptr = front;
            front = front->next;
            deleteNode(ptr);
        }
    }
};
