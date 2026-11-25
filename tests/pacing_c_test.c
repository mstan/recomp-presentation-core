#include "retro_presentation/pacing.h"
int main(void){
    RetroFramePacer pacing={0};
    if(!retro_frame_pacer_init_ratio(&pacing,1000000,1,50))return 1;
    if(retro_frame_pacer_deadline(&pacing,0)!=0)return 2;
    if(retro_frame_pacer_deadline(&pacing,1)!=20000)return 3;
    return 0;
}
