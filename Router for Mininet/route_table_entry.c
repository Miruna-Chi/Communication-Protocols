#ifndef RTE_C
#define RTE_C

#include <stdio.h>
#include <unistd.h>

typedef struct route_table_entry *RTable;

struct route_table_entry {
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
} __attribute__((packed));

sort_rtable (RTable rtable, int n) {

}

void swap(RTable a, RTable b) 
{ 
    route_table_entry t = *a; 
    *a = *b; 
    *b = t; 
} 

int partition (RTable rtable, int low, int high) { 
    route_table_entry pivot = rtable[high];    // pivot 
    int i = (low - 1);  // Index of smaller element 
  
    for (int j = low; j <= high- 1; j++) { 
        // If current element is "smaller" than the pivot 
        if ((rtable[j].prefix & rtable[j].mask < pivot.prefix & pivot.mask) ||
            ((rtable[j].prefix & rtable[j].mask == pivot.prefix & pivot.mask)
            && (rtable[j].prefix < pivot.prefix))) { 

            i++;    // increment index of smaller element 
            swap(&pivot, &rtable[j]); 
        } 
    } 
    swap(&rtable[i + 1], &rtable[high]); 
    return (i + 1); 
} 
  
void quickSort(RTable rtable, int low, int high) 
{ 
    if (low < high) 
    { 
        /* pi is partitioning index, arr[p] is now 
           at right place */
        int pi = partition(arr, low, high); 
  
        // Separately sort elements before 
        // partition and after partition 
        quickSort(arr, low, pi - 1); 
        quickSort(arr, pi + 1, high); 
    } 
} 
  
/* Function to print an array */
void printArray(int arr[], int size) 
{ 
    int i; 
    for (i=0; i < size; i++) 
        printf("%d ", arr[i]); 
    printf("n"); 
} 

#endif