
### upgrade your nvdia driver upper than nvdia-driver-410.
### install cuda from https://developer.nvidia.com/cuda-downloads.
### compile ffmpeg by following config.
```
./configure --enable-gpl --enable-nonfree --enable-libfdk-aac --enable-libx264 --enable-libx265 --enable-filter=delogo --enable-debug --enable-libspeex --enable-shared --enable-pthreads --enable-cuda --enable-cuvid --enable-nvenc --enable-nvdec
```
if you meet some question,you can write your confuse here.
