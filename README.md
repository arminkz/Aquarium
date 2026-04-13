# Aquarium

A digital aquarium featuring Boid simulations driven by GPU compute shaders, built on my custom Vulkan engine.

https://github.com/user-attachments/assets/274f9d5a-687d-45ce-b94f-fda22642ad40


## Building

Requires the Vulkan SDK, CMake 3.10+, and a C++17 compiler.

```bash
cmake -B build
cmake --build build
```

Compile shaders separately:

```bash
./compile_shaders.sh
```
