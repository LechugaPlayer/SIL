# SIL
SIL(simple internet library) is a project made to learn about networking, API design, and C++ while providing a thin abstraction to support both Linux and Windows.

This library will support the most fundamentals features from usual implementations, providing a minimal abstraction(connect, listen, recv, send, etc).
This project was chosen because both Linux and Windows implementations are based on BDS, making it reasonably easy to abstract both libraries into a single one, as well as making the learning
proccess easier, since there are several concepts that overlap, leaving the internal mechanism differences of both implementations to a minimal.

As early said, the idea is to support a minimal set of features, providing a thin abstraction, this allowed me to copy the exact same API from BSD while offering a minimal overhead. There's no
allocation being made inside the library,for example, leaving everything related to memory to the user.

Implementation details:
  -For now I decided to use intptr_t because it keeps the abstraction simple and it avoids allocating memory on the library side.
  -Classes were taking into consideration, although I decided not to go that route because I want to try a procedural style.
  -Abstraction on errors are yet not supported, I will provide this in the future, for now the library prints the errors into the console.
  -Macros are kept at the minimal and only used because it is a must(to decide in which platform the code is being executed)
    
