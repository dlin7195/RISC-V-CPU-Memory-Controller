// BASE COPY --> only L1 cache

#include <cstring>
#include <sstream>
#include <vector>

#include <cassert>


#ifndef _MEM_CTRLR_H_
#include "mem_ctrlr.hpp"
#endif

using namespace std;

#define TEST_DEBUG
#undef TEST_DEBUG

#define DUMP_ALL_CACHE
#undef DUMP_ALL_CACHE


#ifdef TEST_DEBUG
/* 
 * to convert/cast 1D array to 2D array
 */
void dump_cache(int cache_model, int cache_lvl, int sel_blk_idx, uint16_t set_idx, int dump_all, cache_set *my_cache)
{
//   DEBUG_PRINTF(">%s>r2l=%d,my_cache=%p\n",__func__,__LINE__,my_cache);
   switch(cache_model)
   {
      case FA:
      {
         if (dump_all == 1)
         {
            DEBUG_PRINTF(",set_idx=%02d,sel_idx=%02d\n",set_idx, sel_blk_idx);
         }
         for(int b=0; b < CACHE_SETS; b++)
         {
            if (dump_all == 1)
            {
               DEBUG_PRINTF(",fa[%02d]=(%02d,%02d,%03d)\n",b,my_cache[b].tag,my_cache[b].lru_position,my_cache[b].data);
            }
            else
            {
               if(b == sel_blk_idx)
                  DEBUG_PRINTF(",set_idx=%02d,sel_idx=%02d,fa[%02d]=(%02d,%02d,%03d)",set_idx, sel_blk_idx,b,my_cache[b].tag,my_cache[b].lru_position,my_cache[b].data);
            }
         }
         break;
      }
      case SA:
      {
         int max_sa_ways;
         int max_sa_sets;
         cache_set **sa_cache = NULL;
//   DEBUG_PRINTF(">%s>r2l=%d,sa_cache=%p\n",__func__,__LINE__,sa_cache);
         if (dump_all == 1)
         {
            DEBUG_PRINTF(",set_idx=%02d,sel_idx=%02d\n",set_idx, sel_blk_idx);
         }

         switch(cache_lvl)
         {
            case 1:
               max_sa_ways = L1_CACHE_WAYS;
               max_sa_sets = L1_CACHE_SA_SETS;
               sa_cache = (cache_set **)malloc(max_sa_sets * sizeof(cache_set *));
               break;

            case 2:
               max_sa_ways = L2_CACHE_WAYS;
               max_sa_sets = L2_CACHE_SA_SETS;
               sa_cache = (cache_set **)malloc(max_sa_sets * sizeof(cache_set *));
               break;
         }
//   DEBUG_PRINTF(">%s>r2l=%d,sa_cache=%p\n",__func__,__LINE__,sa_cache);
         for(int s=0; s < max_sa_sets; s++)
         {
            sa_cache[s] = (cache_set *)&my_cache[s * max_sa_ways];
//   DEBUG_PRINTF(">%s>r2l=%d,sa_cache[%d]=%p\n",__func__,__LINE__,s,sa_cache[s]);

         }
         for(int s=0; s < max_sa_sets; s++)
         {
            for(int w = 0; w < max_sa_ways; w ++)
            {
               if (dump_all == 1)
               {
                   DEBUG_PRINTF(",sa[%02d][%02d]=(%02d,%02d,%03d)\n", s, w, sa_cache[s][w].tag, sa_cache[s][w].lru_position,sa_cache[s][w].data);
               }
               else
               {
                  if ((s == set_idx ) && (w == sel_blk_idx % max_sa_ways))
                  {
                     {
                        DEBUG_PRINTF(",set_idx=%02d,sel_idx=%02d,sa[%02d][%02d]=(%02d,%02d,%03d)",set_idx, sel_blk_idx, s, w,sa_cache[s][w].tag,sa_cache[s][w].lru_position,sa_cache[s][w].data);
                     }
                  }
               }
            }
         }

         if(sa_cache != NULL)
            free(sa_cache);

         break;
      }

      case DM: 
         break;

      default:
         fprintf(stderr, ">>>%s>>>unknown cache model=%d\n", __func__,cache_model);
         break;

   }

}
#endif



// helper function to access the cache

//    a.  If it is a match (for one set in DM, or any of sets in FA/SA), then it is a
//        hit. You should then access the Data Store to read the actual value and
//        update Data. “Search” should then call “Update” function (details later), and
//        finally return True.

int access_cache(int cache_model, int cache_lvl, int *data, int index, int tag, uint16_t *selected_blk_idx, cache_set *my_cache, uint8_t flag)
{
   bool hit = 0;
   *selected_blk_idx = IDX_INV;

   switch(cache_model)
   {
      case DM: // access cache by simple indexing
      {
         if(tag == my_cache[index].tag)
         {
            hit = 1;

            // hit. You should then access the Data Store to read the actual value and
            // update Data
            switch(flag)
            {
               case FLAG_CURR_MEM_R:
                  *data = my_cache[index].data;
                  break;

               // the Data Store should be updated with the new value in 
               // func cache_access() if it is a hit 
               case FLAG_CURR_MEM_W:
                  my_cache[index].data = *data;
                  break;

               default:
                  fprintf(stderr, ">%s>r2l=%d,unknown flag=%x\n",__func__,__LINE__,flag);
                  break;
            }

            *selected_blk_idx = index;
         }
         break;
      }

      case FA:   // search all sets without any indexing
      {
         for(int i=0; i < CACHE_SETS ; i++)
         {
            if(tag == my_cache[i].tag)
            {
               hit = 1;

               // exit the for loop either on a hit or tag is inv
               switch(flag)
               {
                  case FLAG_CURR_MEM_R:
                     *data = my_cache[i].data;
                     break;

                  // todo_???, should data store be updated with new value if it is hit or miss? --> resolved
                  // the Data Store should be updated with the new value in 
                  // func cache_access() if it is a hit 
                  case FLAG_CURR_MEM_W:
                     my_cache[i].data = *data;
                     break;

                  default:
                     fprintf(stderr, ">%s>r2l=%d,unknown flag=%x\n",__func__,__LINE__,flag);
                     break;
               }
               *selected_blk_idx = i;
               break;
            }
         }
         break;
      }

      case SA:
      {
         int max_ways;
         int w;

         switch(cache_lvl)
         {
            case 1:
               max_ways = CACHE_WAYS;
               break;
            case 2:
               max_ways = L2_CACHE_WAYS;
               break;

         }

         // each set has multiple ways. so p, a start sequence pos in
         // my_cache, is set with calculated (set) index times max_ways
         // w is the counter/index for SA ways
         int p = index * max_ways;
         for(w = 0; w < max_ways ; w++, p++)
         {
            if(tag == my_cache[p].tag)
            {
               hit = 1;

               switch(flag)
               {
                  case FLAG_CURR_MEM_R:
                     *data = my_cache[p].data;
                     break;

                  // the Data Store should be updated with the new value in 
                  // func cache_access() if it is a hit 
                  case FLAG_CURR_MEM_W:
                     my_cache[p].data = *data;
                     break;

                  default:
                     fprintf(stderr, ">%s>r2l=%d,unknown flag=%x\n",__func__,__LINE__,flag);
                     break;
               }

               *selected_blk_idx = p;
               break;
            }
         }

         break;
      }

      default:
         fprintf(stderr,">>>%s>>>unknown cache model=%d\n",__func__,cache_model);
   }

   return hit;
}



// helper function to calculate offset, index, and/or tag
/* 
 *
 */
void calc_off_idx_tag(int model, int level, int addr, uint8_t *offset,uint16_t *idx, uint32_t *tag)
{
   //num_of_off_bits = (uint32_t)log2(BLOCK_SIZE); 
   switch(model)
   {
      case DM:
      {
         struct dm_cache_addr *dm_ca  = (struct dm_cache_addr *)&addr;
         *offset = 0;
         *idx    = (uint16_t)dm_ca->index;
         *tag    = (uint32_t)dm_ca->tag;
         break;
      }
      case FA:
      {
         struct fa_cache_addr *fa_ca  = (struct fa_cache_addr *)&addr;
         *offset = 0;
         *idx    = 0;
         *tag    = (uint32_t) fa_ca->tag;
         break;
      }
      case SA:
      {
         if(level == 1)
         {
            struct sa_L1_cache_addr *sa_L1_ca  = (struct sa_L1_cache_addr *)&addr; 
            *offset = 0;
            *idx    = (uint16_t)sa_L1_ca->index;
            *tag    = (uint32_t)sa_L1_ca->tag;
         }
         else if(level == 2)
         {
            struct sa_L2_cache_addr *sa_L2_ca  = (struct sa_L2_cache_addr *)&addr;
            *offset = 0;
            *idx    = (uint16_t)sa_L2_ca->index;
            *tag    = (uint32_t)sa_L2_ca->tag;
         }

         break;
      }

      default:
         *offset = 0;
         *idx    = 0;
         *tag    = -1;
         fprintf(stderr, ">>>%s>>>unknown cache model=%d\n", __func__,model);
         break;
      
   }
   return;
}



// Update function

// In Update function, you should update the LRU counters. Counters will
// be updated if either there is a “hit”, or when a new line is installed from
// MM.

void Update(int cache_model, int cache_lvl, uint16_t selected_blk_idx, uint16_t set_idx, cache_set *my_cache)
{
   int max_ways = 0;

   // set up max_ways for SA only
   if(cache_model == SA)
   {
      switch(cache_lvl)
      {
         case 1:
            max_ways = CACHE_WAYS;
            break;

         case 2:
            max_ways = L2_CACHE_WAYS;
            break;

         default:
            fprintf(stderr, ">>>%s>>>unknown cache level=%d\n", __func__,cache_lvl);
            break;
      }
   }
   //DEBUG_PRINTF(">>%s>>r2l=%d,selected_blk_idx=%d\n",__func__,__LINE__,selected_blk_idx);
   //DEBUG_PRINTF(">>my_cache[%d].tag=%d\n",selected_blk_idx, my_cache[selected_blk_idx].tag);
   //DEBUG_PRINTF(">>B4>>>my_cache[%d].lru_position=%d\n",selected_blk_idx, my_cache[selected_blk_idx].lru_position);


   // if tag is still inv
   // set lru_position with sequence number in my_cache[] for both SA and FA.
   // selected_blk_idx was the sequence index in my_cache[]
   if(my_cache[selected_blk_idx].tag == -1)
   {
      switch(cache_model)
      {
         case FA:
            my_cache[selected_blk_idx].lru_position = selected_blk_idx;
            break;

         case SA:
            // SA need to coverted selected_blk_idx to local way index within a set.
            my_cache[selected_blk_idx].lru_position = selected_blk_idx % max_ways;
            break;

         default:
            fprintf(stderr, ">>>%s>>>unknown cache model=%d\n", __func__,cache_model);
            break;
      }
      //DEBUG_PRINTF(">%s>r2l=%d>AF>>>my_cache[%d].lru_position=%d\n",__func__,__LINE__,selected_blk_idx, my_cache[selected_blk_idx].lru_position);
   }


   // if hit or miss
   else
   {
      int num_of_v_tag = 0;
      int is_dec_pos = 0;	// to toggle decrementing of block positions
      int old_pos;

      switch(cache_model)
      {
         int blk;
         case FA:
         {
            // find out how many valid tags
            for(blk=0; blk < CACHE_SETS; blk++)
            {
               if(my_cache[blk].tag > -1)
               {
                  num_of_v_tag ++;
               }
            }

            // set the selected blk pos to highest(youngest) of blocks in use
            // back to back hit scenarios
            // already the youngest postition(tail of the queue)
            // no need to update the pos and that of other blk.
            if(my_cache[selected_blk_idx].lru_position < num_of_v_tag - 1)
            {
               old_pos = my_cache[selected_blk_idx].lru_position;
               my_cache[selected_blk_idx].lru_position = num_of_v_tag - 1;
               is_dec_pos = 1;
            }

            if(is_dec_pos == 1)
            {
               // think about the position in a queue
               // update the rest of  blocks that was yougner than selected block 
               for(int blk=0; blk < CACHE_SETS; blk++)
               {
                  if(blk == selected_blk_idx)
                  {
                     continue;
                  }
                  // decrement the pos of the rest of younger blks than old pos selected blk  by one
                  else
                  {
                     // Note: only one entry will be 0 position at any time
                     // 0 will be still 0.
                     if((my_cache[blk].lru_position > 0) && 
                        (my_cache[blk].lru_position > old_pos))
                     {
                        my_cache[blk].lru_position -= 1;
                     }
                  }

               }
            }
            break;
         }
         case SA:
         {
            // each set has multiple way. so p, a start sequence pos in
            // my_cache, is set with calculated (set) index times max_ways
            // w is the index for SA ways.

            // find out how many valid tags
            int p = set_idx * max_ways;
            int w;

            for(w = 0; w < max_ways; w++,p++)
            {
               if(my_cache[p].tag > -1)
               {
                  num_of_v_tag ++;
               }
            }

            // set the selected blk pos to highest(youngest) of blocks in use
            // back to back hit scenarios
            // already the youngest postition(tail of the queue)
            // no need to update the pos and that of other blk.
            if(my_cache[selected_blk_idx].lru_position < num_of_v_tag - 1)
            {
               old_pos = my_cache[selected_blk_idx].lru_position;
               my_cache[selected_blk_idx].lru_position = num_of_v_tag - 1;
               is_dec_pos = 1;
            }

            if(is_dec_pos == 1)
            {
               p = set_idx * max_ways;
               for(w = 0; w < max_ways; w++,p++)
               {
                  // think about the position in a queue
                  // the selected_blk_idx pos should be higest if the code run 
                  // to here. just continue without update.
                  if(p == selected_blk_idx)
                  {
                     continue;
                  }
                  // decrement the pos of the rest of younger blks than old pos of selected blk  by one
                  else
                  {
                     // 0 will be still 0.
                     if((my_cache[p].lru_position > 0) && 
                        (my_cache[p].lru_position > old_pos))
                     {
                        my_cache[p].lru_position -= 1;
                     }
                  }

               }
            }
            break;
         }
         case DM:
         default:
            fprintf(stderr, ">>>%s>>>unknown or unsupport cache model=%d\n",__func__,cache_model);
            break;
      }
   }
   return;
}



// Evict function

// In Evict, three things need to be done: 1- Finding a candidate for
// eviction (finding lru_position ==0 ). 2- Updating the counters (i.e., calling
// Update). 3- Updating the tag and data stores with the new values (read from
// MM). It then returns.

void Evict(int addr, int *data, int cache_model, int cache_lvl, cache_set *my_cache)
{
   uint8_t  offset=0;
   uint16_t index=IDX_INV; 
   uint32_t tag=0;

   uint16_t selected_blk_idx=IDX_INV; 
   int blk;
 
   calc_off_idx_tag(cache_model, cache_lvl, addr, &offset,&index, &tag);

   // 1- Finding a candidate for eviction (finding lru_position ==0 ). 
   switch(cache_model)
   {
      // FA does not use index,
      case FA:
      {
         // find the first encountered block tag still not used yet , inv(-1)
         for(blk=0; blk < CACHE_SETS; blk++)
         {
            // find the block tag still inv(-1)
            if(my_cache[blk].tag == -1)
            {
               selected_blk_idx = blk;
               break;
            }
         }
         // all the blks are aged already
         // find the LRU block 
         if(selected_blk_idx == IDX_INV)
         {
            for(blk=0; blk < CACHE_SETS; blk++)
            {
               if(my_cache[blk].lru_position == 0)
               {
                  selected_blk_idx = blk;
                  break;
               }
            }
         }
         break;
      }

      case SA:
      {
         int w;
         int max_ways;
         int p;

         // assert index against IDX_INV
         //DEBUG_PRINTF(">>%s>>r2l=%d,index=%x,tag=%x\n",__func__,__LINE__,index,tag);
         assert(index != IDX_INV);

         switch(cache_lvl)
         {
            case 1:
               max_ways = CACHE_WAYS;
               break;
            case 2:
               max_ways = L2_CACHE_WAYS;
               break;

         }

         // find the block tag still not used yet , tag is inv(-1).
         // each set has multiple way. so p, a start sequence pos in my_cache,
         // is set with calculated(set) index times max_ways
         // w is the index for SA ways.
         p = index * max_ways;
         for(w = 0; w < max_ways ; w++, p++)
         {
            if(my_cache[p].tag == -1)
            {
               selected_blk_idx = p;
               break;
            }
         }

         // all the blks are aged already
         // find the LRU block 
         if(selected_blk_idx == IDX_INV)
         {
            // In SA, each set has multiple ways. so p, a start sequence pos in my_cache,
            // is set with calculated(set) index times max_ways
            // w is the counter for set ways.
            p = index * max_ways;
            for(w = 0; w < max_ways ; w++, p++)
            {
               if(my_cache[p].lru_position == 0)
               {
                  selected_blk_idx = p;
                  break;
               }
            }
         }
         break;
      }
     
      case DM:
         selected_blk_idx = index;
         break;

      default:
         fprintf(stderr, ">>>%s>>>unknown cache model=%d\n", __func__,cache_model);
         break;
   }
    

   // 2- Updating the counters (i.e., calling "Update" function)
   if((cache_model == FA) || (cache_model == SA))
   {
      Update(cache_model, cache_lvl, selected_blk_idx, index, my_cache);
   }


   // 3- Updating the tag and data stores with the new values (read from MM). It
   // then returns.
   // Note: new value are read from MM before Evict() is called.
   my_cache[selected_blk_idx].tag = (int)tag;
   my_cache[selected_blk_idx].data = *data;


// for debugging purposes
#ifdef TEST_DEBUG
   if((cache_model == FA) || (cache_model == SA))
   {
#ifdef DUMP_ALL_CACHE
      int dump_all = 1;  // 0: dump selected cache content 
                         // 1: dump selected cache content 
#else
      int dump_all = 0;  // 0: dump selected cache content 
                         // 1: dump selected cache content 
#endif
      dump_cache(cache_model, cache_lvl, selected_blk_idx, index, dump_all,  my_cache);
   }
#endif

   return;
}



// CacheMiss function

// In CacheMiss, it has to read the data from the main memory (i.e.,
// accessing MM[adr]). It then calls Evict, and then returns.

void CacheMiss(int addr, int *data, int cache_model, int cache_lvl, cache_set *my_cache, int *my_mem)
{
   // it has to read the data from the main memory (i.e.,accessing MM[adr]). 
   assert(addr < MEM_SIZE);
   *data = my_mem[addr];

   // It then calls Evict
   Evict(addr, data, cache_model, cache_lvl, my_cache);

   // and then returns.
   return;

}



// Search function

// 2.  In Search, using the address, your code should first calculate the
// correct offset and index, and the tag.  Then, it should access the cache
// (i.e., by using the index to read the tag store array).
//
// 3.  If it is a direct-mapped cache, then “Search” should just use the index to
// access the cache and check the stored tag with the new tag. If it is a FA
// cache, then it should search ALL sets. If it is a SA, it should do both.
//
//    a.  If it is a match (for one set in DM, or any of sets in FA/SA), then it is a
//        hit. You should then access the Data Store to read the actual value and
//        update Data. Search should then call Update function (details later), and
//        finally return True(1).
//
//    b.  Otherwise, it is a miss. In that case, your code should return false(0).


int Search(int *addr, int *data, int cache_model, int cache_lvl, cache_set *my_cache, uint8_t flag)
{
   bool hit = 0;

   uint8_t  offset=0;
   uint16_t index=IDX_INV;
   uint32_t tag=0;

   uint16_t selected_blk_idx=IDX_INV;

   // using the address, your code should first calculate the
   // correct offset and index, and the tag. 
   calc_off_idx_tag(cache_model, cache_lvl, *addr, &offset,&index, &tag);

   //Then, it should access the cache
   hit = access_cache(cache_model, cache_lvl, data, index, tag, &selected_blk_idx, my_cache, flag);


// debugging
#ifdef TEST_DEBUG
   switch(cache_model) 
   {
      case DM:
      {
         if(flag & FLAG_CURR_MEM_R)
         {
            DEBUG_PRINTF(",idx=%02d,tag=%03d,hit=%d",index, tag, hit);
         }
         else
         {
            if(flag & FLAG_CURR_MEM_W)
            {
               DEBUG_PRINTF(",idx=%02d,tag=%03d,hit=%d",index, tag, hit);
               //DEBUG_PRINTF(",idx=%d,tag=%x",index, tag);
            }
         }
         break;
      }
      case FA:
      case SA:
      {
         // load only
         if(flag & FLAG_CURR_MEM_R)
         {
            DEBUG_PRINTF(",idx=%02d,tag=%03d,hit=%d",index,tag,hit);
         }
         else
         {
            if(flag & FLAG_CURR_MEM_W)
            {
               DEBUG_PRINTF(",idx=%02d,tag=%03d,hit=%d",index, tag, hit);
               //DEBUG_PRINTF(",idx=%d,tag=%x",index, tag);
            }
         }
         break;
      }
      default:
         break;
   }
#endif


   if(hit == 1)
   {
      
      // update Data by accessing the data store to read the actual value --> done in access_cache()
      // call Update() to update LRU counters --> for both LW and SW

      if((cache_model == FA) || (cache_model == SA))
      {
         Update(cache_model, cache_lvl, selected_blk_idx, index, my_cache);

// debugging
#ifdef TEST_DEBUG
      {
#ifdef DUMP_ALL_CACHE
      int dump_all = 1;  // 0: dump selected cache content 
                         // 1: dump selected cache content 
#else
      int dump_all = 0;  // 0: dump selected cache content 
                         // 1: dump selected cache content 
#endif
            dump_cache(cache_model, cache_lvl, selected_blk_idx, index, dump_all, my_cache);
      }
#endif

      }
      
   }

   return hit;
}



// SW function

/*
SW Function Design:
1.  Same as LW, it first calls Search function (same as LW for steps 1-3).
The only difference is that if it is a hit, instead of reading the value from
the Data store, the Data Store should be updated with the new value.
2.  Returning from Search,
   a.  If it is a “hit”, you have to also update the MM[adr] (i.e.,
       write-through strategy).
   b.  If it is a miss, you again update the MM[adr] but do not allocate a new
       line in your cache (i.e., write-no-allocate strategy).
3.  In either case (hit or miss), you assign Status=1 and Data=0.
*/

int SW(int addr, int *data, int type, cache_set *my_cache, int *my_mem, uint8_t flag)
{
   int status;
   int cache_lvl = 1;
   bool hit = 0;

   // 1. (adr, data) where both are passed by reference, and a boolean output: hit.
   // The only difference is that if it is a hit, instead of reading the value from
   // the Data store, the Data Store should be updated with the new value in 
   // func cache_access() if hit is 1.
   hit = Search(&addr, data, type, cache_lvl, my_cache, flag);

   // 2.
   //  a. If it is a hit, you have to also update the MM[adr] (i.e.,
   //     write-through strategy).
   if(hit == 1)
   {
      assert(addr < MEM_SIZE);
      my_mem[addr] = *data;
   }

   //  b. If it is a miss, you again update the MM[adr] but do not allocate a new
   //     line in your cache (i.e., write-no-allocate strategy).
   else
   {
      // again update the MM[adr]
      assert(addr < MEM_SIZE);
      my_mem[addr] = *data;
      // *miss += 1; //todo_???,
   }

   //3.  In either case (hit or miss), you assign Status=1 and Data=0.
   status = 1;
   *data = 0;

   return status;
}



// LW function

/* 
LW Function Design:
1.  It should call Search function, which has two inputs: (adr, data) where both are passed by reference, and a boolean output: hit.
2.  In Search, using the address, your code should first calculate the correct offset and index, and the tag.  Then, it should access the cache (i.e., by using the index to read the tag store array).
3.  If it is a direct-mapped cache, then Search should just use the index to access the cache and check the stored tag with the new tag. If it is a FA cache, then it should search ALL sets. If it is a SA, it should do both.
a.  If it is a match (for one set in DM, or any of sets in FA/SA), then it is a hit. You should then access the Data Store to read the actual value and update Data. Search should then call Update function (details later), and finally return True.
b.  Otherwise, it is a miss. In that case, your code should return false.
4.  Returning from Search, there are two scenarios: if it is a hit, then your controller should assign status = 1. If it is a miss, it should assign Status = -3 (this is to model the read delay in the MM. i.e., in this case, it takes four cycles to read from the memory). It then calls CacheMiss.
a.  In CacheMiss, it has to read the data from the main memory (i.e., accessing MM[adr]). It then calls Evict, and then returns.
b.  In Evict, three things need to be done: 1- Finding a candidate for eviction (finding lru_position ==0 ). 2- Updating the counters (i.e., calling Update). 3- Updating the tag and data stores with the new values (read from MM). It then returns.
5.  In Update function, you should update the LRU counters. Counters will be updated if either there is a hit, or when a new line is installed from MM.
*/

int LW(int addr, int *data, int type, cache_set *my_cache, int *my_mem, int *miss, uint8_t flag)
{
   int status;
   int cache_lvl = 1;
   bool hit = 0;

   // call Search
   hit = Search(&addr, data, type, cache_lvl, my_cache, flag);

   // Returning from Search, there are two scenarios: if it is a hit, then
   // your controller should assign status = 1.
   if(hit == 1)
   {
      status = 1;
   }
   // If it is a miss, it should assign Status = -3 (this is to model the read
   // delay in the MM. i.e., in this case, it takes four cycles to read from the
   // memory). It then calls CacheMiss.
   else
   {
      status = -3;
      CacheMiss(addr, data, type, cache_lvl, my_cache, my_mem);
      // increment miss counter for no hit
      *miss += 1;
   }

   return status;
}



// memory controller function

// in your memory controller you need to implement your FSM, LW, SW, and MM. 
int memory_controller(int cur_mem_r, int cur_mem_w, int *curr_data, int curr_adr, int prev_sts, int *miss, int type, cache_set *my_cache, int *my_mem)
{
   int status = prev_sts;
   uint8_t flag;

   // controller checks the (previous) Status and does the following based on this value:
   // Status is inputted in memory_driver.cpp while loop

   switch(status)
   {
      // If it is 1, it checks control signals (i.e., MemR and MemW). If either is one,
      // it calls the corresponding function (i.e., Load or Store). 

      case 1:
         // DEBUG_PRINTF("mem_r,mem_w,adr,data=(%d,%d,%03d,%03d)",cur_mem_r, cur_mem_w, curr_adr, *curr_data ); 
         if(cur_mem_r == 1)
         {
            flag = FLAG_CURR_MEM_R;
            status = LW(curr_adr, curr_data, type, my_cache, my_mem, miss, flag);
            // DEBUG_PRINTF(",ret data=%03d,miss=%d\n",*curr_data, *miss);
         }
         else if(cur_mem_w == 1)
         {
            flag = FLAG_CURR_MEM_W;
            status = SW(curr_adr, curr_data, type, my_cache, my_mem, flag);
            // DEBUG_PRINTF(",ret data=%03d,\n",*curr_data);
         }
         break;

      // If it is zero, it outputs data=MM[adr] and Status=1.
      case 0:
         assert(curr_adr < MEM_SIZE);
         //todo_??? is the below MM[adr] needed since curr_data was updated in the previous cycle?
         *curr_data = my_mem[curr_adr];   
         status = 1;
         break;

      // If it is negative, it increments the Status by one. 
      default:
         if(status < 0)
            status += 1;
         break;
   }    

   return status;
}

