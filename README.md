[![progress-banner](https://backend.codecrafters.io/progress/http-server/121c341e-b9dc-458f-9e5e-dd4e7aa5ae9a)](https://app.codecrafters.io/users/nippongun?r=2qF)
This is a learning repository. I did the CodeCrafter challenge to build an HTTP server in C/C++ 20. 

## Design
My approach was to separate the endpoints to their own respective Wrappers, allowing every endpoint to  The Server itself is self-contained ie singelton. 

## Learned
I learned how to build an HTTP Server from scratch using various libraries. The course explained Web Sockets, TCP/UDP, gzip compression, REST, and generally HTTP.
Moreover, I learned how to apply the learned knowledge using C++ 20. It introduced certain STL libraries such as socket.h, arpa/inet etc.
For design, I learned how to keep code maintainable and modular.

## What I would do differently
I would have a look at the thread safety of the server. Additionally, the implementation would require more refactoring regarding try/catch.
But more importantly I would redesign the server. Instead of a wrapper for every endpoint - which ultimately was shared by all threads - every thread (client) receives its own Dispatch stack (mapping an endpoint to a handler i.e. lambda function). This avoids OOP, reduces thread problems and is equally maintainable. Furthermore, the HTTP implementation is rather ad hoc and could benefit from a unified HTTP approach (collection of structs and enums).

