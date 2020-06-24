#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "route_table.h"

void make_entry (RTable rtable, uint32_t prefix, uint32_t next_hop, 
                uint32_t mask, int interface, int i) {
    
    rtable[i].prefix = prefix;
    rtable[i].next_hop = next_hop;
    rtable[i].mask = mask;
    rtable[i].interface = interface;

}

int binary_search(RTable rtable, int l, int r, uint32_t dest_ip) 
{ 
    if (r >= l) { 
        int mid = l + (r - l) / 2; 

        if ((rtable[mid].prefix & rtable[mid].mask) 
            == (dest_ip & rtable[mid].mask))
            return mid; 
  
        if ((rtable[mid].prefix & rtable[mid].mask)
            > (dest_ip & rtable[mid].mask)) 
            return binary_search(rtable, l, mid - 1, dest_ip); 

        return binary_search(rtable, mid + 1, r, dest_ip); 
    } 
  
    return -1; 
} 

RTable get_best_route (RTable rtable, 
    uint32_t dest_ip, int size, FILE *out) {
                                        
    int position;
    position = binary_search(rtable, 0, size - 1, dest_ip);
    if(position == -1)
        return NULL;
    position++;
    while ((dest_ip & rtable[position].mask)
        == (rtable[position].mask & rtable[position].prefix)
        && position <= size) {
            position++;
    }

    return &rtable[position - 1];
}

void swap(RTable a, RTable b) { 
    struct route_table_entry t = *a; 
    *a = *b; 
    *b = t; 
} 

int partition (RTable rtable, int low, int high) { 
    struct route_table_entry pivot = rtable[high];    // pivot 
    int i = (low - 1);  // Index of smaller element 
  
    for (int j = low; j <= high- 1; j++) { 
        // If current element is "smaller" than the pivot 
        if (((rtable[j].prefix & rtable[j].mask) < (pivot.prefix & pivot.mask))
            || (((rtable[j].prefix & rtable[j].mask) 
            == (pivot.prefix & pivot.mask))
            && (rtable[j].mask < pivot.mask))) { 

            i++;    // increment index of smaller element 
            swap(&rtable[i], &rtable[j]); 
        } 
    } 
    swap(&rtable[i + 1], &rtable[high]); 
    return (i + 1); 
} 
  
void quick_sort(RTable rtable, int low, int high) { 
    if (low < high) { 

        int pi = partition(rtable, low, high); 

        quick_sort(rtable, low, pi - 1); 
        quick_sort(rtable, pi + 1, high); 
    } 
} 
