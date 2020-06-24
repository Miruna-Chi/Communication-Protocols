typedef struct route_table_entry *RTable;

struct route_table_entry {
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
} __attribute__((packed));

void make_entry(RTable rtable, uint32_t prefix, uint32_t next_hop, 
    				uint32_t mask, int interface, int i);

int binary_search (RTable rtable, int l, int r, uint32_t dest_ip);

RTable get_best_route (RTable rtable, uint32_t dest_ip, int size, FILE *out);

void swap(RTable a, RTable b);

int partition (RTable rtable, int low, int high);

void quick_sort(RTable rtable, int low, int high);
