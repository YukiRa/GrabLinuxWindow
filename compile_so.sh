#! /bin/sh
gcc grabwindow.cpp dsimple.c clientwin.c -fPIC -shared -o ./libgrabwindow.so \
-I ./include -L ./lib -lavformat -lavdevice -lavfilter -lavcodec -lavutil -lpthread -lswscale -lswresample
