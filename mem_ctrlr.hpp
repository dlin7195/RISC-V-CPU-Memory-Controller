// BASE COPY --> only for L1 cache

#ifndef _MEM_CTRLR_H_
#define  _MEM_CTRLR_H_

#define DEBUG
#undef DEBUG
#ifdef DEBUG
    #define DEBUG_PRINTF(...) fprintf(stderr, __VA_ARGS__)
#else
    #define DEBUG_PRINTF(...) do {} while (0)
#endif

#define CACHE_SETS 16 
#define MEM_SIZE 2048
#define CACHE_WAYS 4 //  4 ways and 4 sets
#define BLOCK_SIZE 1 // bytes per block
#define DM 0
#define FA 1
#define SA 2
// L1 cache
#define L1_CACHE_SA_SETS 4  // level 1 cache (an 4-way set associative with 4 sets) 
#define L1_CACHE_WAYS    4 
#define L1_CACHE_SETS    L1_CACHE_SA_SETS * L1_CACHE_WAYS   // level 1 cache (an 4-way set associative with 1 sets) 
// L2 cache
#define L2_CACHE_SA_SETS 8  // level 2 cache (an 8-way set associative with 8 sets) 
#define L2_CACHE_WAYS    8 
#define L2_CACHE_SETS    L2_CACHE_SA_SETS * L2_CACHE_WAYS   // level 2 cache (an 8-way set associative with 8 sets) 

#define IDX_INV   0xFFFF

#define FLAG_CURR_MEM_R   0x01
#define FLAG_CURR_MEM_W   0x02


struct dm_cache_addr
{
   //unsigned  offset:0;   // block size was one byte
   unsigned  index:4;   // 16 sets
   unsigned  tag:28;
};

struct sa_L2_cache_addr
{
   //unsigned  offset:0;
   unsigned  index:3;   // 8 sets
   unsigned  tag:29;
};

struct sa_L1_cache_addr
{
   //unsigned  offset:0;
   unsigned  index:2;   // 4 sets
   unsigned  tag:30;
};

struct fa_cache_addr
{
   //unsigned  offset:0;
   //unsigned  index:0;
   unsigned  tag:32;
};

struct cache_set
{
	int tag; // you need to compute offset and index to find the tag.
	int lru_position; // for FA and SA only
	int data; // the actual data stored in the cache/memory
	// add more things here if needed
};

int memory_controller(int cur_mem_r, int cur_mem_w, int *curr_data, int curr_adr, int prev_sts, int *miss, int type, cache_set *my_cache, int *my_mem);

#endif
