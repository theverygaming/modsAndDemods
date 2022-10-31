# PSKmod
QPSK modulator and decoder for soft symbols in softDecoder.cpp

dependencies: ``volk`` and ``ncurses``

```
git clone --recurse-submodules https://github.com/theverygaming/PSKmod.git
cd PSKmod
mkdir build && cd build
cmake ..
make
g++ ../softDecoder.cpp -lncurses -o softDecoder
```