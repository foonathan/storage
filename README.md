storage - A library providing optional and variant classes
==========================================================

*Note: This project was created out of fun in 2014 to test out Biicode, which was a C/C++ package manager.
It would be sad if the could would be permanently deleted, so I've rescued it and decided to put it on Github.
The implementation is entirely header-only, just download them somewhere appropriate.
Maybe someone will find it useful.*

Motivation
----------
The reasons for optional and variant types are plenty and well-known.

Optionals provide a great way to handle types that just may not be not there, without actually creating them. It can act just like a pointer to an object, but without the need to allocate heap memory for it.

Variants are type-safe unions automatically taking care of proper construction and destruction. In addition, they allow error checking.

I have made my own implementation, that uses C++11 features to create this small and useful library.

Overview
--------
The library mainly consists of two classes, variant and optional. In addition there are low-level building blocks used in the implementation, like pointer conversion functions and an unchecked, low-level raw_storage. The latter is just a typedef around *std::aligned_union* with creation and access functions but no checking whatsoever.

*optional* and *variant* both provide similar operations, mainly as free functions. This includes a *get* function to access the currently stored value - if any, methods to check whether there is one, and an *emplace* function to create a new value. In addition, there is a useful *try_get* function that either returns the stored value if there is one or a given replacement value, and a *visit* function.

The *visit* function allows to apply a *visitor* to an optional or variant. A *visitor* is a callable object, that provides an *operator()* for the type of an optional/each type of a variant. For an optional, it calls it only if there is a value stored. For a variant it calls the equivalent overload of it for the type currently stored. This allows easily doing operations on them and is a simple and fast implementation of the Anti-If idiom.

See the two examples in the example folder for further information.

Design Rationale
----------------
*optional* is modeled after a pointer. Its only member functions are operators, providing similar operations as pointers. The raw storage comes from *std::aligned_union*.

*variant* is somewhat similar to optional. But instead of containing only one type, it can contain multiple but only one at a time - like a native union. Unlike Boost.Variant it does not provide a never-empty guarantee. Due to exception safety issues, it is kind of difficult to implement. Boost solved it via using heap memory in case of emergency. One of my design goals was speed and the least overhead possible, so I have not provided it. Besides, I don't think it is that necessary.

Exception safety was a big concern of mine. The emplace function of variant can provide the strong exception safety as long as the type is nothrow moveable. In addition, I implemented some functions in a new way. Unlike *std::experimental::optional* for example, both emplace functions assign a new value to the old one if the type matches. This delegates the problem of exception safety to the move assignment operator which is often noexcept and thus provides the strong exception safety in total. *swap* was challenging, too, and can only provide a bad form of basic exception safety when confronted with a throwing move constructor. Look at the code documentation and the code itself to get more information about the called functions.

The currently stored type inside a variant is identified via an index. The index corresponds to the index of the type in the template argument list. There are functions to get the index for a special type; no type is identified via the special value corresponding to the size of the template argument list.

*variant*'s internal member functions heavily rely on the visit function. They are called for destroying, copying and other operations. To avoid this - small - overhead, there is a special check whether all types are trivial. If they are, these operations are skipped at compile time and replaced with bitwise copy thus creating only small overhead.

Error checking is currently done via the *assert* macro.
