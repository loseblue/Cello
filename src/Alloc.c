#include "Cello.h"

struct CelloHeader* header(var self) {
  return (struct CelloHeader*)((char*)self - sizeof(struct CelloHeader));
}

var header_init(struct CelloHeader* head, var type, int alloc) {
  
  head->type = type;
  
#if CELLO_ALLOC_CHECK == 1
  head->alloc = (var)(intptr_t)alloc;
#endif
  
#if CELLO_MAGIC_CHECK == 1
  head->magic = (var)CELLO_MAGIC_NUM;
#endif

  return ((char*)head) + sizeof(struct CelloHeader);
}

static const char* Alloc_Name(void) {
  return "Alloc";
}

static const char* Alloc_Brief(void) {
  return "Memory Allocation";
}

static const char* Alloc_Description(void) {
  return
    "The `Alloc` class can be used to override how memory is allocated for a "
    "given data type. By default memory is allocated using `malloc` along with "
    "the `Size` class to determine the amount of memory to allocate.";
    "\n\n"
    "A custom allocator should be careful to also initialise the header for the "
    "allocated memory using the function `CelloHeader_Init`. Cello objects "
    "without a header wont be recognised as such as so will throw errors when "
    "used with Cello functions."
    "\n\n"
    "Allocating memory via `alloc` avoids the garbage collector and so much be "
    "manually freed using `dealloc`."
  ;
}

/* TODO */
static const char* Alloc_Examples(void) {
  return "";
}

/* TODO */
static const char* Alloc_Methods(void) {
  return "";
}

var Alloc = Cello(Alloc, Instance(Doc, 
  Alloc_Name, Alloc_Brief, Alloc_Description, Alloc_Examples, Alloc_Methods));


var alloc_stk(var type, var head, var data, size_t size) {
  header_init(head, type, AllocStack);
  memcpy((char*)head + sizeof(struct CelloHeader), data, size);
  var self = (char*)head + sizeof(struct CelloHeader);  
  return self;
}

var alloc(var type) {
  
  struct Alloc* a = type_instance(type, Alloc);
  if (a and a->alloc) {
    return a->alloc();
  }
  
  struct CelloHeader* head = calloc(1,
    sizeof(struct CelloHeader) + size(type));
  
#if CELLO_MEMORY_CHECK == 1
  if (head is NULL) {
    throw(OutOfMemoryError, "Cannot create new '%s', out of memory!", type);
  }
#endif
  
  return header_init(head, type, AllocHeap);
}

void dealloc(var self) {

  struct Alloc* a = instance(self, Alloc);
  if (a and a->dealloc) {
    a->dealloc(self);
    return;
  }

  struct CelloHeader* head = (struct CelloHeader*)(
    (char*)self - sizeof(struct CelloHeader));

#if CELLO_ALLOC_CHECK == 1
  if (self is NULL) {
    throw(ResourceError, "Attempt to deallocate NULL!"); 
  }

  if (header(self)->alloc is (var)AllocStatic) {
    throw(ResourceError,
      "Attempt to deallocate %$ "
      "which was allocated statically!", self); 
  }
  
  if (header(self)->alloc is (var)AllocStack) {
    throw(ResourceError,
      "Attempt to deallocate %$ "
      "which was allocated on the stack!", self); 
  }
  
  if (header(self)->alloc is (var)AllocData) {
    throw(ResourceError,
      "Attempt to deallocate %$ "
      "which was allocated inside a data structure!", self); 
  }
#endif
  
#if CELLO_ALLOC_CHECK == 1
  //memset(header(self), 0, sizeof(struct CelloHeader) + size(type_of(self)));
#endif
  
  free(head);
  
}

static const char* New_Name(void) {
  return "New";
}

static const char* New_Brief(void) {
  return "Construction and Destruction";
}

static const char* New_Description(void) {
  return
    "The `New` class allows the user to define constructors and destructors "
    "for a type, accessible via `new` and `del`. Objects allocated with `new` "
    "are allocated on the heap and also registered with the Garbage Collector "
    "this means technically it isn't required to call `del` on them as they "
    "will be cleaned up at a later date."
    "\n\n"
    "The `new_root` function can be called to register a variable with the "
    "Garbage Collector but to indicate that it will be manually destructed "
    "with `del` by the user. This should be used for variables that wont be "
    "reachable by the Garbage Collector such as those in the data segment or "
    "only accessible via vanilla C structures."
    "\n\n"
    "It is also possible to simply call the `construct` and `destruct` "
    "functions if you wish to construct an already allocated object without "
    "interacting with the Garbage Collector."
    "\n\n"
    "Constructors should assume that memory is zero'd for an object but "
    "nothing else."
    "\n\n"
    "The `new` function takes a list of `var` as it's arguments. This means "
    "if you want to pass it native C types you should wrap them using `$`.";
}

/* TODO */
static const char* New_Examples(void) {
  return "";
}

/* TODO */
static const char* New_Methods(void) {
  return "";
}

var New = Cello(New,
  Instance(Doc, New_Name, New_Brief, 
    New_Description, New_Examples, New_Methods));

var construct_with(var self, var args) {
  struct New* n = instance(self, New);
  if (n and n->construct_with) {
    n->construct_with(self, args);
  } else if (len(args) == 1) {
    assign(self, get(args, $I(0)));
  }
  return self;
}

var destruct(var self) {
  struct New* n = instance(self, New);
  if (n and n->destruct) {
    n->destruct(self);
  }
  return self;
}

var new_with(var type, var args) { 
  
  var self = construct_with(alloc(type), args);

#if CELLO_GC == 1
  gc_add(self, false);
#endif

  return self;
}

var new_root_with(var type, var args) { 
  
  var self = construct_with(alloc(type), args);
  
#if CELLO_GC == 1
  gc_add(self, true);
#endif
  
  return self;
}

void del(var self) {
  dealloc(destruct(self));

#if CELLO_GC == 1
  gc_rem(self);
#endif
}


static const char* Copy_Name(void) {
  return "Copy";
}

static const char* Copy_Brief(void) {
  return "Copyable";
}

static const char* Copy_Description(void) {
  return
    "The `Copy` class can be used to override the behaviour of an object when "
    "a copy is made of it. By default the `Copy` class allocates a new empty "
    "object of the same type and uses the `Assign` class to set the "
    "contents. The copy is then registered with the Garbage Collector as if it "
    "had been constructed with `new`. This means when using manual memory "
    "management a copy must be deleted manually."
    "\n\n"
    "If the `copy` class is overridden then the implementor may manually have "
    "to register the object with the Garbage Collector if they wish for it to "
    "be tracked. For this they should call `gc_add` with the new object."
    "\n\n"
    "By convention `copy` follows the semantics of `Assign`, which typically "
    "means a _deep copy_ should be made, and that an object will create a "
    "copy of all of the sub-objects it references or contains - although this "
    "could vary depending on the type's overridden behaviours.";
}

/* TODO */
static const char* Copy_Examples(void) {
  return "";
}

/* TODO */
static const char* Copy_Methods(void) {
  return "";
}

var Copy = Cello(Copy,
  Instance(Doc,
    Copy_Name, Copy_Brief, Copy_Description, 
    Copy_Examples, Copy_Methods));

var copy(var self) {
  
  struct Copy* c = instance(self, Copy);
  if (c and c->copy) {
    return c->copy(self); 
  }
  
  var obj = assign(alloc(type_of(self)), self);
    
#if CELLO_GC == 1
  gc_add(obj, true);
#endif
    
  return obj;
  
}

