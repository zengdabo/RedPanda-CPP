# Dependancy
 
 Red Panda C++ need Qt 5 (>=5.12) to build.

# Windows
 I build Red Panda Cpp with the latest gcc and mingw-w64, distributed by msys2 mingw-w64. Visual C++  and other version of gcc may not work.

 - Install msys2 (https://www.msys2.org)
 - Use msys2's pacman to install mingw-w64-x86_64-qt5 and mingw-w64-x86_64-gcc
 - Install qtcreator
 - Use qtcreator to open Red_Panda_CPP.pro

# Linux  (Ubuntu/Debian)


## 1. install complier

```text
apt install gcc g++ make gdb gdbserver 
```

## 2. install QT5 and lib

```text
apt install qtbase5-dev qttools5-dev-tools  libicu-dev libqt5svg5-dev  git qterminal
```

## 3. download source

```
git clone https://gitee.com/royqh1979/RedPanda-CPP.git
```

### 4. compile 

```
cd cd RedPanda-CPP/
qmake Red_Panda_CPP.pro 
make -j8
sudo make install
```

## 5. run

```
RedPandaIDE
```
