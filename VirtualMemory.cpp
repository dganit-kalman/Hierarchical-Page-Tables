#include "VirtualMemory.h"
#include "PhysicalMemory.h"

struct Frame{
    word_t parent;
    word_t num_page;
    uint64_t num_frame;
};

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

word_t find_dist (word_t page_swapped_in,word_t p){
    word_t abc = page_swapped_in > p ? page_swapped_in-p : p-page_swapped_in;
    word_t result = (PAGE_SIZE - abc) < abc ? (PAGE_SIZE-abc): abc;
    return result;


}


void VMinitialize() {
    clearTable(0);
}
void dfsFrame(int depth,  word_t* unused_frame, word_t* empty_frame , Frame* old, uint64_t
page_swap_in, word_t p, uint64_t address, uint64_t base_address, word_t parent){

    if(depth == TABLES_DEPTH){
        if(base_address!= address && address < NUM_FRAMES && (old->num_frame == 0 || find_dist
        (page_swap_in,p) > find_dist(page_swap_in, old->num_page))){
            old->num_page = p;
            old->num_frame = address;
            old->parent = parent;
        }
        return;
    }
    if(*empty_frame > 0){
        return;
    }
    bool is_empty= true;
    word_t  current_frame_index = 0;
    p <<= OFFSET_WIDTH;

    for (int i = 0; i <PAGE_SIZE ; ++i) {
        p = ((p >> OFFSET_WIDTH) << OFFSET_WIDTH) + i;


        PMread(address*PAGE_SIZE +i,&current_frame_index);
        if(current_frame_index > *unused_frame && depth <TABLES_DEPTH ){
            *unused_frame = current_frame_index;
        }

        if(current_frame_index != 0){
            is_empty = false;
            if(depth < TABLES_DEPTH){
                dfsFrame(depth+1 , unused_frame, empty_frame,old, page_swap_in,p,
                         (uint64_t)current_frame_index, base_address,address);
            }
        }
    }
    if(is_empty && base_address != address && *empty_frame <= 0){
        *empty_frame = (word_t)address;
        old->parent = parent;
    }
}


int findFrame(uint64_t * frame, uint64_t page_swap_in, uint64_t base_address){
    word_t emptyFrame{}, unused_frame{};
    Frame old{};
    dfsFrame(0, &unused_frame, &emptyFrame, &old, page_swap_in, 0, 0, base_address, 0 );
    if(emptyFrame > 0){
        word_t value{};
        for (int i = 0; i <PAGE_SIZE ; ++i) {
            PMread((uint64_t )old.parent*PAGE_SIZE+i,&value);
            if(value == emptyFrame){
                PMwrite((uint64_t)old.parent*PAGE_SIZE+i, 0);
                break;
            }

        }
        *frame = (uint64_t)emptyFrame;
    } else if(unused_frame + 1 < NUM_FRAMES ){
        *frame = (uint64_t)unused_frame+1;
    } else if(old.num_frame > 0){
        PMevict(old.num_frame, old.num_page);
        uint64_t offset = old.num_page & (PAGE_SIZE - 1);
        PMwrite(old.parent * PAGE_SIZE + offset, 0);
        *frame = old.num_frame;
    } else{
        return 0;
    }
    if (*frame == 0) {
        return 0;
    }
    return 1;

}


int traverse(uint64_t virtualAddress, uint64_t *frame, uint64_t *offset){

    int num_address = (VIRTUAL_ADDRESS_WIDTH+OFFSET_WIDTH-1)/OFFSET_WIDTH;
    uint64_t addresses[num_address];
//    word_t word{};
    uint64_t baseAddress{};
    uint64_t page_num = virtualAddress >> OFFSET_WIDTH;

    for (int i=0; i<num_address; i++){
        addresses[i]=(virtualAddress & (PAGE_SIZE-1));
        virtualAddress >>= OFFSET_WIDTH;
    }
    uint64_t frame1{};


    for(int j = num_address-1; j > 0; --j){
        word_t word{};

        PMread(baseAddress*PAGE_SIZE+addresses[j], &word);
        if(word == 0){
            if(findFrame(&frame1, page_num, baseAddress) == 0){
                return 0;
            }
            clearTable(frame1);
            PMwrite(baseAddress*PAGE_SIZE+addresses[j],(word_t)frame1);
            word = (word_t)frame1;
        }
        baseAddress = (uint64_t )word;
    }
    *frame = baseAddress;
    *offset = addresses[0];

    PMrestore(baseAddress, page_num);
    return 1;

}


int VMread(uint64_t virtualAddress, word_t* value) {
    uint64_t frame{}, offset{};
    if(traverse(virtualAddress, &frame, &offset) == 0)
        return 0;
    PMread(frame * PAGE_SIZE + offset, value);

    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    uint64_t frame{}, offset{};
    if(traverse(virtualAddress, &frame, &offset) == 0)
        return 0;
    PMwrite(frame * PAGE_SIZE + offset, value);

    return 1;
}
