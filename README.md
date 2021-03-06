# Trillek Engine C
| Windows (VStudio 2019)  | Semaphore (GCC) |
|-------------------------|-----------------|
| [![Build status](https://ci.appveyor.com/api/projects/status/809xi9ukwo7sgsip?svg=true)](https://ci.appveyor.com/project/adam4813/tec-hem9u) | [![Build Status](https://semaphoreci.com/api/v1/trillek-team/tec/branches/master/shields_badge.svg)](https://semaphoreci.com/trillek-team/tec) |


# Requirements
TEC requires cmake 3.9 and a few libraries GLFW3, GLM, ASIO, Protobuf, GLEW, Lua, Bullet, Dear ImGui, Spdlog and OpenAL which can be installed most easily via [vcpkg](#vcpkg)

# VCPKG
1. `git clone https://github.com/Microsoft/vcpkg.git` (If you already have **VCPKG**, go to [building](#building))
1. Navigate to the `vcpkg/` directory.
	 1. Windows: `./bootstrap-vcpkg.bat` or Others: `./bootstrap-vcpkg..sh`
	 1. **MacOS MAYBE** `sudo installer -pkg /Library/Developer/CommandLineTools/Packages/macOS_SDK_headers_for_macOS_10.14.pkg -target /` if `bootstrap-vcpkg` fails see [here](https://donatstudios.com/MojaveMissingHeaderFiles) for more help
	 1. **Linux** `apt-get install libgl1-mesa-dev xorg-dev libglu1-mesa-dev libxinerama-dev libxcursor-dev`
	 1. **OPTIONAL** `./vcpkg integrate install`
	 1. `./vcpkg install asio bullet3 glew glfw3 glm lua openal-soft protobuf zlib spdlog imgui`

# Building
1. `git submodule update --init` in the root directory.
1. `mkdir build/` in to root directory
1. `cd build/`
1. Follow platform specific instructions 
   1. Linux (G++ 7 or CLang 4) **Unsupported at this moment due to have cmake is setup**
       1. **INSTALL REQUIRED LIBS** bullet, glew, glfw3, glm, asio, lua, openal-soft, spdlog, Dear ImGui, and protobuf. Some of these will need versions not in your distribution (just ask for help in the IRC.)
            1. If you are on Ubuntu/Debian/etc. (something with `apt`):
                1. Run `apt-get install libglew-dev libglfw3 libglm-dev libasio-dev`
                2. Run `apt-get install liblua5.2-dev libopenal-dev  libbullet-dev`
                3. Run `apt-get install libprotobuf-dev protobuf-compiler libspdlog-dev`
            2. If you are on Fedora/RHEL/etc. (something with `rpm`):
			    1. *instructions coming soon to a README near you*
            3. If you are on Arch/etc. (something with `pacman`):
			    1. Run `pacman -S glew glfw-x11 glm asio lua52 openal bullet protobuf spdlog`
       1. `cmake ..` in the build directory
       1. `make tec` in the build directory
   1. Linux VCPKG (G++ 7 or CLang 4)
      1. `cmake -DCMAKE_TOOLCHAIN_FILE=**VCPKG_DIR**/scripts/buildsystems/vcpkg.cmake ..` in the build directory
      1. `make` in the build directory
   1. Windows (Visual Studio 2019)
      1. Run cmake-gui setting the source line to the root directory and the build line to the build directory.
      1. Hit configure and select `Specify toolchain file for cross-compiling` using `**VCPKG_DIR**/scripts/buildsystems/vcpkg.cmake`.
      1. Click generate; then open and build the solution in Visual Studio.
      1. In the project properties for `trillek-client` change the `Debugging`->`Working Directory` to `$(SolutionDir)..\`.
      1. **Potentially** Download and install oalinst.zip (OpenAL installer) http://openal.org/downloads/ and install it.
   1. MaxOS X 10.14
      1. `brew install bullet`
      1. `cmake -DCMAKE_TOOLCHAIN_FILE=**VCPKG_DIR**/scripts/buildsystems/vcpkg.cmake ..` in the build directory
      1. `make` in the build directory
1. Run it from `tec/`

### Unit tests
To generate the unit tests, follow the same instructions that before, but set to true the flag BUILD_TESTS_TEC
