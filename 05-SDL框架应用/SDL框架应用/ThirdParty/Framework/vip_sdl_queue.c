//
//  jdc_sdl_queue.c
//  JDCFFPlayer
//
//  Created by ChenJidong on 17/4/4.
//  Copyright © 2017年 jidong. All rights reserved.
//

#include "vip_sdl_queue.h"

struct VIPSDLPacketQueue {
    void *first_pk;
    void *last_pk;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
    int quit;
};

typedef struct VIPQueueNode {
    struct VIPQueueNode *next;
    void *data;
}VIPQueueNode;

//创建包队列
VIPSDLPacketQueue *vip_packet_queue_alloc()
{
    return (VIPSDLPacketQueue *)av_mallocz(sizeof(VIPSDLPacketQueue));
}

//往队列中东西
void vip_packet_queue_init(VIPSDLPacketQueue *queue)
{
    memset(queue, 0, sizeof(VIPSDLPacketQueue));
    queue->mutex = SDL_CreateMutex();
    queue->cond = SDL_CreateCond();
}  

int vip_packet_queue_push(VIPSDLPacketQueue *queue , void *packet)
{
    VIPQueueNode *listNode = (VIPQueueNode *)malloc(sizeof(VIPQueueNode));
    
    if (!listNode) {
        return -1;
    }
    
    listNode->data = packet;
    listNode->next = NULL;
    
    SDL_LockMutex(queue->mutex);
    
    if (queue->first_pk == NULL) {
        queue->first_pk = listNode;
    }else{
        ((VIPQueueNode *)queue->last_pk)->next = listNode;
    }
    
    queue->last_pk = listNode;
    queue->size ++;
    
    
    SDL_CondSignal(queue->cond);
    SDL_UnlockMutex(queue->mutex);
    
    return 0;
}

int vip_packet_queue_size(VIPSDLPacketQueue *queue)
{
    return queue->size;
}

void *vip_packet_queue_front(VIPSDLPacketQueue *queue)
{
    SDL_LockMutex(queue->mutex);
    
    AVPacket *pkt = NULL;
    if (queue->first_pk) {
        return ((VIPQueueNode *)queue->first_pk)->data;
    }
    
    SDL_UnlockMutex(queue->mutex);
    
    return pkt;
}

void *vip_packet_queue_pop(VIPSDLPacketQueue *queue)
{
    void *data = NULL;
    SDL_LockMutex(queue->mutex);
    
    if (queue->first_pk) {
        VIPQueueNode *firstPkl = queue->first_pk;
        data = firstPkl->data;
        queue->first_pk = firstPkl->next;
        queue->size--;
        
        free(firstPkl);
    }
    
    SDL_UnlockMutex(queue->mutex);
    
    return data;
}

int vip_packet_queue_get_packet(VIPSDLPacketQueue *queue , void **pkg , int block)
{
    int ret;
    
    SDL_LockMutex(queue->mutex);
    
    while (1) {
        if (queue->quit) {
            ret = -1;
            break;
        }
        
        if (queue->first_pk) {
            *pkg = vip_packet_queue_pop(queue);
            ret = 1;
            break;
        }else if(block){
            SDL_CondWait(queue->cond, queue->mutex);
        }else{
            ret = 0;
            break;
        }
    }
    
    SDL_UnlockMutex(queue->mutex);
    
    return ret;
}
