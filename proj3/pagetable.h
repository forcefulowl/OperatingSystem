typedef struct {
		int pid;
		int page;
} FRAME;

typedef struct {
		int frameNo;
		bool valid;
		bool dirty;
} PTE;

typedef struct {
		int hitCount;
		int missCount;
		int tlbHitCount;
		int tlbMissCount;
} STATS;

typedef struct {
		PTE entry[NUM_PROC][NUM_PAGE];
		STATS stats;
} PT;

typedef struct {
		int entry[2]; 
		int frameNo[2];
		bool valid[2];
		int lru;
} TLB;

int pagefault_handler(int pid, int pageNo, char type, bool *hit);


extern int lru_flag;
extern int frameIndex;
extern int PageCounter;
extern int clock_Flag[NUM_FRAME];
extern int tlbsetIndex[NUM_PROC][TLB_ENTRY / 2];