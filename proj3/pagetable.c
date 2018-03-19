#include <stdio.h>
#include "vm.h"
#include "disk.h"
#include "pagetable.h"
#include "list.h"

PT pageTable;  
FRAME frame_table[NUM_FRAME];  
TLB tlb[NUM_PROC][TLB_ENTRY/2];


int PageCounter = 0;
int lru_flag = 0;
int clock_Flag[NUM_FRAME] = {0};
int tlbsetIndex[NUM_PROC][TLB_ENTRY / 2] = {1}; 
int frameIndex;

// If the page is in TLB (tlb hit), return frame number.
// otherwise (tlb miss), return -1.
int is_TLB_hit(int pid, int pageNo, int type)
{
	if (TLB_ENTRY == 0)
	return -1; 
	int i;
	int tag1 = tlb[pid][pageNo % (TLB_ENTRY/2)].entry[0];
	int tag2 = tlb[pid][pageNo % (TLB_ENTRY/2)].entry[1];

	if(tlb[pid][pageNo % (TLB_ENTRY / 2)].valid[0] == true && tag1 == pageNo)
	{
		tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)] = 0;
		if(type == 'W')
		{
			pageTable.entry[pid][pageNo].dirty = true;
		}
		if(replacementPolicy == LRU)
		{
			frameIndex = pageTable.entry[pid][pageNo].frameNo;
			
			lru_flag = 1;
			page_replacement();
		}
		if(replacementPolicy == CLOCK)
		{
			frameIndex = pageTable.entry[pid][pageNo].frameNo;
			
			clock_Flag[frameIndex] = 1;
		}
		return pageTable.entry[pid][pageNo].frameNo;
	}
	else if(tlb[pid][pageNo % (TLB_ENTRY / 2)].valid[1] == true && tag2 == pageNo)
	{
		tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)] = 1;
		if(type == 'W')
		{
			pageTable.entry[pid][pageNo].dirty = true;
		}
		if(replacementPolicy == LRU)
		{
			frameIndex = pageTable.entry[pid][pageNo].frameNo;
			
			lru_flag = 1;
			page_replacement();
		}
		if(replacementPolicy == CLOCK)
		{
			frameIndex = pageTable.entry[pid][pageNo].frameNo;
			
			clock_Flag[frameIndex] = 1;
		}
		return pageTable.entry[pid][pageNo].frameNo;			
	}
	return -1;
}


// If the page is already in the physical memory (page hit), return frame number.
// otherwise (page miss), return -1.
int is_page_hit(int pid, int pageNo, char type)
{
	if(pageTable.entry[pid][pageNo].valid == true)
	{		
		if(TLB_ENTRY != 0)
		{
			tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)] = 1 - tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)];
			tlb[pid][pageNo % (TLB_ENTRY / 2)].valid[tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)]] = true;
			tlb[pid][pageNo % (TLB_ENTRY / 2)].entry[tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)]] = pageNo;
			tlb[pid][pageNo % (TLB_ENTRY / 2)].frameNo[tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)]] = pageTable.entry[pid][pageNo].frameNo;
		}
		if(type == 'W')
		{
			pageTable.entry[pid][pageNo].dirty = true;
		}
		if(replacementPolicy == LRU)
		{
			frameIndex = pageTable.entry[pid][pageNo].frameNo;
			lru_flag = 1;

			page_replacement();
		}
		if(replacementPolicy == CLOCK)
		{
			frameIndex = pageTable.entry[pid][pageNo].frameNo;
			clock_Flag[frameIndex] = 1;
		}
		return pageTable.entry[pid][pageNo].frameNo;
	}
	return -1;
}

int pagefault_handler(int pid, int pageNo, char type, bool *hit)
{
	*hit = false;
	if(PageCounter == NUM_FRAME)
	{
		lru_flag = 2;
		int rep_frame = page_replacement();
		int rep_pid = frame_table[rep_frame].pid;
		int rep_page = frame_table[rep_frame].page;
		int i;
		pageTable.entry[rep_pid][rep_page].valid = false;
		if (TLB_ENTRY != 0)
		{
			for(i = 0; i < 2; i++)
			{
				if(tlb[rep_pid][rep_page % (TLB_ENTRY / 2)].entry[i] == rep_page)
				{
					tlb[rep_pid][rep_page % (TLB_ENTRY / 2)].valid[i] = false;
					tlbsetIndex[rep_pid][rep_page % (TLB_ENTRY / 2)] = 1 - i;
				}
			}
		}
		if(pageTable.entry[rep_pid][rep_page].dirty == true)
		{
			disk_write(rep_frame, rep_pid, rep_page);
			pageTable.entry[rep_pid][rep_page].dirty = false;
		}
		pageTable.entry[pid][pageNo].valid = true;
		pageTable.entry[pid][pageNo].frameNo = rep_frame;
		disk_read(pageTable.entry[pid][pageNo].frameNo, pid, pageNo);
		frame_table[pageTable.entry[pid][pageNo].frameNo].pid = pid;
		frame_table[pageTable.entry[pid][pageNo].frameNo].page = pageNo;
		pageTable.entry[pid][pageNo].dirty = false;
		if(type == 'W')
		{
			pageTable.entry[pid][pageNo].dirty = true;
		}
		if (TLB_ENTRY != 0)
		{	
			tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)] = 1 - tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)];
			tlb[pid][pageNo % (TLB_ENTRY / 2)].valid[tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)]] = true;
			tlb[pid][pageNo % (TLB_ENTRY / 2)].entry[tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)]] = pageNo;
			tlb[pid][pageNo % (TLB_ENTRY / 2)].frameNo[tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)]] = pageTable.entry[pid][pageNo].frameNo;
		} 
		return pageTable.entry[pid][pageNo].frameNo;
	}
	else 
	{
		lru_flag = 3;
		pageTable.entry[pid][pageNo].valid = true;
		pageTable.entry[pid][pageNo].frameNo = PageCounter;
		if(replacementPolicy == LRU)
		{
			frameIndex = pageTable.entry[pid][pageNo].frameNo;
			page_replacement();
		}
		if(replacementPolicy == CLOCK)
		{
			frameIndex = pageTable.entry[pid][pageNo].frameNo;
			clock_Flag[frameIndex] = 1;
		}
		disk_read(pageTable.entry[pid][pageNo].frameNo, pid, pageNo);
		frame_table[pageTable.entry[pid][pageNo].frameNo].pid = pid;
		frame_table[pageTable.entry[pid][pageNo].frameNo].page = pageNo;
		pageTable.entry[pid][pageNo].dirty = false;
		if(type == 'W')
		{
			pageTable.entry[pid][pageNo].dirty = true;
		}
		if (TLB_ENTRY != 0)
		{	
			tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)] = 1 - tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)];
			tlb[pid][pageNo % (TLB_ENTRY / 2)].valid[tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)]] = true;
			tlb[pid][pageNo % (TLB_ENTRY / 2)].entry[tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)]] = pageNo;
			tlb[pid][pageNo % (TLB_ENTRY / 2)].frameNo[tlbsetIndex[pid][pageNo % (TLB_ENTRY / 2)]] = pageTable.entry[pid][pageNo].frameNo;
		}
		PageCounter++;
		return pageTable.entry[pid][pageNo].frameNo;
	}
}



int MMU(int pid, int addr, char type, bool *hit, bool *tlbhit)
{

	int frameNo;
	int pageNo = (addr >> 8);
	int offset = addr - (pageNo << 8);
	int physicalAddr;

	if(pageNo > NUM_PAGE) 
	{
		printf("invalid page number (NUM_PAGE = 0x%x): pid %d, addr %x\n", NUM_PAGE, pid, addr);
		return -1;
	}
	if(pid > NUM_PROC-1) 
	{
		printf("invalid pid (NUM_PROC = %d): pid %d, addr %x\n", NUM_PROC, pid, addr);
		return -1;
	}

//TLB hit 
		frameNo = is_TLB_hit(pid, pageNo, type);
		if(frameNo > -1)
		{
			*tlbhit = true;
			*hit = true;
			pageTable.stats.hitCount++;
			pageTable.stats.tlbHitCount++;
			physicalAddr = (frameNo << 8) + offset;
			return physicalAddr;
		} 
		else *tlbhit = false;

//Page hit 
		frameNo = is_page_hit(pid, pageNo, type);
		if(frameNo > -1)
		{
			*hit = true;
			pageTable.stats.hitCount++;
			pageTable.stats.tlbMissCount++;
			physicalAddr = (frameNo << 8) + offset;
			return physicalAddr;
		}

//Pagefault 
		frameNo = pagefault_handler(pid, pageNo, type, hit);
		pageTable.stats.missCount++;
		pageTable.stats.tlbMissCount++;
		physicalAddr = (frameNo << 8) + offset;
		return physicalAddr;
}

void pt_print_stats()
{
	int req = pageTable.stats.hitCount + pageTable.stats.missCount;
	int hit = pageTable.stats.hitCount;
	int miss = pageTable.stats.missCount;
	int tlbHit = pageTable.stats.tlbHitCount;
	int tlbMiss = pageTable.stats.tlbMissCount;

	printf("Request: %d\n", req);
	printf("Page Hit: %d (%.2f%%)\n", hit, (float) hit*100 / (float)req);
	printf("Page Miss: %d (%.2f%%)\n", miss, (float) miss*100 / (float)req);
	printf("TLB Hit: %d (%.2f%%)\n", tlbHit, (float) tlbHit*100 / (float)req);
	printf("TLB Miss: %d (%.2f%%)\n", tlbMiss, (float) tlbMiss*100 / (float)req);
}

void pt_init()
{
	int i,j;

	pageTable.stats.hitCount = 0;
	pageTable.stats.missCount = 0;
	pageTable.stats.tlbHitCount = 0;
	pageTable.stats.tlbMissCount = 0;

	for(i = 0; i < NUM_PROC; i++) 
	{
		for(j = 0; j < NUM_PAGE; j++) 
		{
			pageTable.entry[i][j].valid = false;
		}
	}
}

void tlb_init()
{
	int i, j;
	for (i = 0; i < NUM_PROC; i++)
	{
		for(j = 0; j < TLB_ENTRY / 2; j++)
		{
			tlb[i][j].valid[0] = tlb[i][j].valid[1] = false;
			tlb[i][j].lru = 0;
		}
	}
}

