[![Build Status](https://travis-ci.org/zacharyselk/hog2.svg?branch=PDB=refactor)](https://travis-ci.org/zacharyselk/hog2)


<!-- ABOUT THE PROJECT -->
## About the Project

HOG2 is a cross platform search framework for a variety of algorithms and domains that runs on all modern operating systems including ios.


<!-- GETTING STARTED -->
## Getting Started

To get started with the HOG2 applications run the following commands

### Prerequisites

#### Linux
On Ubuntu run the following command:
```sh
# apt install build-essential libglu1-mesa-dev freeglut3-dev
```

On Debian run:
```sh
# apt install git libglu1-mesa-dev freeglut3-dev
```

On Arch run:
```sh
# pacman -S git base-devel mesa glu freeglut
```

On CentOs and Fedora run:
```sh
# yum install git make gcc-c++ mesa-libGL-devel mesa-libGLU-devel freeglut-devel
```

#### MacOS
TODO

#### Windows 10:
TODO


### Installation

To build the project, you must first download the source code:
`$ git clone https://github.com/nathansttt/hog2.git`

Then traverse to the build directory with:
```sh
cd hog2/build/gmake
```

Finally build the project with make:
```sh
make
```

After this completes the binaries can be found under `../../bin/release/`. To fully install the programs to /usr/local/bin, run `sudo make install` under the `hog2/build/gmake/` directory; to uninstall run `sudo make uninstall` in the same directory. The install location can be changed with `make install prefix=</path/to/dir>`.


## Usage

This framework consists of many different applications, each has specific usage information under `apps/<app_name>`.



## Licence

HOG2 is open source software licensed under the [MIT license](LICESNSE.txt)