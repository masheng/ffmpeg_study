#include <QCoreApplication>

#include "player.h"

//audio delay
//video refresh

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

//            char *path="/home/ms/Desktop/a.rmvb";
//        char *path="/home/ms/Desktop/b.flv";
    //    char *path="/home/ms/Desktop/a.mp3";
            char *path="/home/ms/Desktop/hui.avi";
//        char *path="/home/ms/Desktop/mori.mp4";
    //        char *path="http://tvhd.ak.live.cntv.cn/cache/1_/seg0/index.m3u8";

    init(path);

    return a.exec();
}

