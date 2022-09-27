#include "grabwindow.h"
#include <string.h>
#include <iostream>
#include <signal.h>
#include <unistd.h>
//#include <time.h>

bool _run = true;
void sigint_handler(int sig)
{
	if(sig ==SIGINT)
	{
		printf("ctrl + c!\n");
		_run = false;
	}
}


int main()
{
    GrabWindow *g = new GrabWindow();
    g->setFunctionEnableStat(true);
    g->setRenderWindowName("百度一下，你就知道 - Google Chrome");
    g->setVideoFileName("test.mp4");
    g->setSceneStopStat(false);
    //开始录制
    g->setSimuRunStat(true);

    while (_run)
    {
        if(getchar())
	{
	    //结束录制
            g->setSceneStopStat(true);
        }
    }
    sleep(1);
    std::cout<<"test end"<<std::endl;
    return 0;
}
