#include "Cello.h"

static const char* Map_Name(void) {
  return "Map";
}

static const char* Map_Brief(void) {
  return "Balanced Binary Tree";
}

/* TODO */
static const char* Map_Description(void) {
  return "";
}

/* TODO */
static const char* Map_Examples(void) {
  return "";
}

/* TODO */
static const char* Map_Methods(void) {
  return "";
}

struct Map {
  var root;
  var ktype;
  var vtype;
  size_t ksize;
  size_t vsize;
  size_t nitems;
};

static bool Map_Is_Red(struct Map* m, var node);

static var* Map_Left(struct Map* m, var node) {
  return (var*)((char*)node + 0 * sizeof(var));
}

static var* Map_Right(struct Map* m, var node) {
  return (var*)((char*)node + 1 * sizeof(var));
}

static var Map_Get_Parent(struct Map* m, var node) {
  var ptr = *(var*)((char*)node + 2 * sizeof(var));
  return (var)(((uintptr_t)ptr) & (~1));
}

static void Map_Set_Parent(struct Map* m, var node, var ptr) {
  if (Map_Is_Red(m, node)) {
    *(var*)((char*)node + 2 * sizeof(var)) = (var)(((uintptr_t)ptr) | 1);
  } else {
    *(var*)((char*)node + 2 * sizeof(var)) = ptr;
  }
}

static var Map_Key(struct Map* m, var node) {
  return (char*)node + 3 * sizeof(var) + sizeof(struct CelloHeader);
}

static var Map_Val(struct Map* m, var node) {
  return (char*)node + 3 * sizeof(var) +
    sizeof(struct CelloHeader) + m->ksize +
    sizeof(struct CelloHeader);
}

static void Map_Set_Color(struct Map* m, var node, bool col) {
  var ptr = Map_Get_Parent(m, node);
  if (col) {
    *(var*)((char*)node + 2 * sizeof(var)) = (var)(((uintptr_t)ptr) | 1);
  } else {
    *(var*)((char*)node + 2 * sizeof(var)) = ptr;
  }
}

static bool Map_Get_Color(struct Map* m, var node) {
  if (node is NULL) { return 0; }
  var ptr = *(var*)((char*)node + 2 * sizeof(var));
  return ((uintptr_t)ptr) & 1;
}

static void Map_Set_Black(struct Map* m, var node) {
  Map_Set_Color(m, node, false);
}

static void Map_Set_Red(struct Map* m, var node) {
  Map_Set_Color(m, node, true);
}

static bool Map_Is_Red(struct Map* m, var node) {
  return Map_Get_Color(m, node);
}

static bool Map_Is_Black(struct Map* m, var node) {
  return not Map_Get_Color(m, node);
}

static var Map_Alloc(struct Map* m) {
  var node = calloc(1, 3 * sizeof(var) + 
    sizeof(struct CelloHeader) + m->ksize + 
    sizeof(struct CelloHeader) + m->vsize);
  
#if CELLO_MEMORY_CHECK == 1
  if (node is NULL) {
    throw(OutOfMemoryError, "Cannot allocate Map entry, out of memory!");
  }
#endif
  
  var key = header_init((struct CelloHeader*)(
    (char*)node + 3 * sizeof(var)), m->ktype, AllocData);
  var val = header_init((struct CelloHeader*)(
    (char*)node + 3 * sizeof(var) +
    sizeof(struct CelloHeader) + m->ksize), m->vtype, AllocData);
  
  *Map_Left(m, node) = NULL;
  *Map_Right(m, node) = NULL;
  Map_Set_Parent(m, node, NULL);
  Map_Set_Red(m, node);
  
  return node;
}

static void Map_Set(var self, var key, var val);

static void Map_New(var self, var args) {
  struct Map* m = self;
  m->ktype = get(args, $I(0));
  m->vtype = get(args, $I(1));
  m->ksize = size(m->ktype);
  m->vsize = size(m->vtype);
  m->nitems = 0;
  m->root = NULL;

  size_t nargs = len(args);
  if (nargs % 2 isnt 0) {
    throw(FormatError, 
      "Received non multiple of two argument count to Map constructor.");
  }
  
  for(size_t i = 0; i < (nargs-2)/2; i++) {
    var key = get(args, $I(2+(i*2)+0));
    var val = get(args, $I(2+(i*2)+1));
    Map_Set(m, key, val);
  }
  
}

static void Map_Clear_Entry(struct Map* m, var node) {
  if (node isnt NULL) {
    Map_Clear_Entry(m, *Map_Left(m, node));
    Map_Clear_Entry(m, *Map_Right(m, node));
    destruct(Map_Key(m, node));
    destruct(Map_Val(m, node));
    free(node);
  }
}

static void Map_Clear(var self) {
  struct Map* m = self;
  Map_Clear_Entry(m, m->root);
  m->nitems = 0;
  m->root = NULL;
}

static void Map_Del(var self) {
  struct Map* m = self;
  Map_Clear(self);
}

static var Map_Key_Subtype(var self) {
  struct Map* m = self;  
  return m->ktype;
}

static var Map_Val_Subtype(var self) {
  struct Map* m = self;  
  return m->vtype;
}

static void Map_Assign(var self, var obj) {
  struct Map* m = self;
  Map_Clear(self);
  m->ktype = key_subtype(obj);
  m->vtype = val_subtype(obj);
  m->ksize = size(m->ktype);
  m->vsize = size(m->vtype);
  foreach (key in obj) {
    Map_Set(self, key, get(obj, key));
  }
}

static var Map_Iter_Init(var self);
static var Map_Iter_Next(var self, var curr);

static var Map_Copy(var self) {
  struct Map* m = self;
  
  var r = new(Map, m->ktype, m->vtype);
  
  var curr = Map_Iter_Init(self);
  while (curr isnt NULL) {
    var node = (char*)curr - sizeof(struct CelloHeader) - 3 * sizeof(var);
    set(r, Map_Key(m, node), Map_Val(m, node));
    curr = Map_Iter_Next(self, curr);
  }
  
  return r;
}

static bool Map_Mem(var self, var key);
static var Map_Get(var self, var key);

static bool Map_Eq(var self, var obj) {
  
  foreach (key in obj) {
    if (not Map_Mem(self, key)) { return false; }
    if (neq(get(obj, key), Map_Get(self, key))) { return false; }
  }
	
  foreach (key in self) {
    if (not mem(obj, key)) { return false; }
    if (neq(get(obj, key), Map_Get(self, key))) { return false; }
  }
	
  return true;
}

static size_t Map_Len(var self) {
  struct Map* m = self;
  return m->nitems;
}

static bool Map_Mem(var self, var key) {
  struct Map* m = self;
  key = cast(key, m->ktype);
  
  var node = m->root;
  while (node isnt NULL) { 
    int c = cmp(Map_Key(m, node), key);
    if (c is 0) { return true; }
    node = c < 0 ? *Map_Left(m, node) : *Map_Right(m, node);
  }
  
  return false;
}

static var Map_Get(var self, var key) {
  struct Map* m = self;
  key = cast(key, m->ktype);
  
  var node = m->root;
  while (node isnt NULL) {
    int c = cmp(Map_Key(m, node), key);
    if (c is 0) { return Map_Val(m, node); }
    node = c < 0 ? *Map_Left(m, node) : *Map_Right(m, node);
  }
  
  return throw(KeyError, "Key %$ not in Map!", key);
}

static var Map_Maximum(struct Map* m, var node) {
  while (*Map_Right(m, node) isnt NULL) {
    node = *Map_Right(m, node);
  }
  return node;
}

static var Map_Sibling(struct Map* m, var node) {
  
  if (node is NULL or Map_Get_Parent(m, node) is NULL) {
    return NULL;
  }
  
  if (node is *Map_Left(m, Map_Get_Parent(m, node))) {
    return *Map_Right(m, Map_Get_Parent(m, node));
  } else {
    return *Map_Left(m, Map_Get_Parent(m, node));
  }
}

static var Map_Grandparent(struct Map* m, var node) {
  if ((node isnt NULL) and (Map_Get_Parent(m, node) isnt NULL)) {
    return Map_Get_Parent(m, Map_Get_Parent(m, node));
  } else {
    return NULL;
  }
}

static var Map_Uncle(struct Map* m, var node) {
  var gpar = Map_Grandparent(m, node);
  if (gpar is NULL) { return NULL; }
  if (Map_Get_Parent(m, node) is *Map_Left(m, gpar)) {
    return *Map_Right(m, gpar);
  } else {
    return *Map_Left(m, gpar);
  }
}

void Map_Replace(struct Map* m, var oldn, var newn) {
  if (Map_Get_Parent(m, oldn) is NULL) {
    m->root = newn;
  } else {
    if (oldn is *Map_Left(m, Map_Get_Parent(m, oldn))) {
      *Map_Left(m, Map_Get_Parent(m, oldn)) = newn;
    } else {
      *Map_Right(m, Map_Get_Parent(m, oldn)) = newn;
    }
  }
  if (newn isnt NULL) {
    Map_Set_Parent(m, newn, Map_Get_Parent(m, oldn));
  }
}

static void Map_Rotate_Left(struct Map* m, var node) {
  var r = *Map_Right(m, node);
  Map_Replace(m, node, r);
  *Map_Right(m, node) = *Map_Left(m, r);
  if (*Map_Left(m, r) isnt NULL) {
      Map_Set_Parent(m, *Map_Left(m, r), node);
  }
  *Map_Left(m, r) = node;
  Map_Set_Parent(m, node, r);
}

static void Map_Rotate_Right(struct Map* m, var node) {
  var l = *Map_Left(m, node);
  Map_Replace(m, node, l);
  *Map_Left(m, node) = *Map_Right(m, l);
  if (*Map_Right(m, l) isnt NULL) {
    Map_Set_Parent(m, *Map_Right(m, l), node);
  }
  *Map_Right(m, l) = node;
  Map_Set_Parent(m, node, l);
}

static void Map_Set_Fix(struct Map* m, var node) {
  
  while (true) {
  
    if (Map_Get_Parent(m, node) is NULL) {
      Map_Set_Black(m, node);
      return;
    }
    
    if (Map_Is_Black(m, Map_Get_Parent(m, node))) { return; }
    
    if ((Map_Uncle(m, node) isnt NULL)
    and (Map_Is_Red(m, Map_Uncle(m, node)))) {
      Map_Set_Black(m, Map_Get_Parent(m, node));
      Map_Set_Black(m, Map_Uncle(m, node));
      Map_Set_Red(m, Map_Grandparent(m, node));
      node = Map_Grandparent(m, node);
      continue;
    }
    
    if ((node is *Map_Right(m, Map_Get_Parent(m, node))) 
    and (Map_Get_Parent(m, node) is *Map_Left(m, Map_Grandparent(m, node)))) {
      Map_Rotate_Left(m, Map_Get_Parent(m, node));
      node = *Map_Left(m, node);
    }
    
    else
    
    if ((node is *Map_Left(m, Map_Get_Parent(m, node))) 
    and (Map_Get_Parent(m, node) is *Map_Right(m, Map_Grandparent(m, node)))) {
      Map_Rotate_Right(m, Map_Get_Parent(m, node));
      node = *Map_Right(m, node);
    }
    
    Map_Set_Black(m, Map_Get_Parent(m, node));
    Map_Set_Red(m, Map_Grandparent(m, node));
    
    if (node is *Map_Left(m, Map_Get_Parent(m, node))) {
      Map_Rotate_Right(m, Map_Grandparent(m, node));
    } else {
      Map_Rotate_Left(m, Map_Grandparent(m, node));
    }
    
    return;
  
  }
  
}

static void Map_Set(var self, var key, var val) {
  struct Map* m = self;
  key = cast(key, m->ktype);
  val = cast(val, m->vtype);
  
  var node = m->root;
  
  if (node is NULL) {
    var node = Map_Alloc(m);
    assign(Map_Key(m, node), key);
    assign(Map_Val(m, node), val);
    m->root = node;
    m->nitems++;
    Map_Set_Fix(m, node);
    return;
  }
  
  while (true) {
    
    int c = cmp(Map_Key(m, node), key);
    
    if (c is 0) {
      assign(Map_Key(m, node), key);
      assign(Map_Val(m, node), val);
      return;
    }
    
    if (c < 0) {
    
      if (*Map_Left(m, node) is NULL) {
        var newn = Map_Alloc(m);
        assign(Map_Key(m, newn), key);
        assign(Map_Val(m, newn), val);
        *Map_Left(m, node) = newn;
        Map_Set_Parent(m, newn, node);
        Map_Set_Fix(m, newn);
        m->nitems++;
        return;
      }
      
      node = *Map_Left(m, node);
    }
      
    if (c > 0) {
    
      if (*Map_Right(m, node) is NULL) {
        var newn = Map_Alloc(m);
        assign(Map_Key(m, newn), key);
        assign(Map_Val(m, newn), val);
        *Map_Right(m, node) = newn;
        Map_Set_Parent(m, newn, node);
        Map_Set_Fix(m, newn);
        m->nitems++;
        return;
      }
      
      node = *Map_Right(m, node);
    }
    
  }
  
}

static void Map_Rem_Fix(struct Map* m, var node) {
 
  while (true) {
  
    if (Map_Get_Parent(m, node) is NULL) { return; }
    
    if (Map_Is_Red(m, Map_Sibling(m, node))) {
      Map_Set_Red(m, Map_Get_Parent(m, node));
      Map_Set_Black(m, Map_Sibling(m, node));
      if (node is *Map_Left(m, Map_Get_Parent(m, node))) {
          Map_Rotate_Left(m, Map_Get_Parent(m, node));
      } else {
          Map_Rotate_Right(m, Map_Get_Parent(m, node));
      }
    }
    
    if (Map_Is_Black(m, Map_Get_Parent(m, node))
    and Map_Is_Black(m, Map_Sibling(m, node))
    and Map_Is_Black(m, *Map_Left(m, Map_Sibling(m, node)))
    and Map_Is_Black(m, *Map_Right(m, Map_Sibling(m, node)))) {
      Map_Set_Red(m, Map_Sibling(m, node));
      node = Map_Get_Parent(m, node);
      continue;
    }
    
    if (Map_Is_Red(m, Map_Get_Parent(m, node))
    and Map_Is_Black(m, Map_Sibling(m, node))
    and Map_Is_Black(m, *Map_Left(m, Map_Sibling(m, node)))
    and Map_Is_Black(m, *Map_Right(m, Map_Sibling(m, node)))) {
      Map_Set_Red(m, Map_Sibling(m, node));
      Map_Set_Black(m, Map_Get_Parent(m, node));
      return;
    }
    
    if (Map_Is_Black(m, Map_Sibling(m, node))) {
    
      if (node is *Map_Left(m, Map_Get_Parent(m, node))
      and Map_Is_Red(m, *Map_Left(m, Map_Sibling(m, node)))
      and Map_Is_Black(m, *Map_Right(m, Map_Sibling(m, node)))) {
        Map_Set_Red(m, Map_Sibling(m, node));
        Map_Set_Black(m, *Map_Left(m, Map_Sibling(m, node)));
        Map_Rotate_Right(m, Map_Sibling(m, node));
      }
      
      else 
      
      if (node is *Map_Right(m, Map_Get_Parent(m, node))
      and Map_Is_Red(m, *Map_Right(m, Map_Sibling(m, node)))
      and Map_Is_Black(m, *Map_Left(m, Map_Sibling(m, node)))) {
        Map_Set_Red(m, Map_Sibling(m, node));
        Map_Set_Black(m, *Map_Right(m, Map_Sibling(m, node)));
        Map_Rotate_Left(m, Map_Sibling(m, node));
      }
    
    }
    
    Map_Set_Color(m, 
      Map_Sibling(m, node),  
      Map_Get_Color(m, Map_Get_Parent(m, node)));
    
    Map_Set_Black(m, Map_Get_Parent(m, node));
    
    if (node is *Map_Left(m, Map_Get_Parent(m, node))) {
        Map_Set_Black(m, *Map_Right(m, Map_Sibling(m, node)));
        Map_Rotate_Left(m, Map_Get_Parent(m, node));
    } else {
        Map_Set_Black(m, *Map_Left(m, Map_Sibling(m, node)));
        Map_Rotate_Right(m, Map_Get_Parent(m, node));
    }
    
    return;
  }

}

static void Map_Rem(var self, var key) {
  struct Map* m = self;
  
  key = cast(key, m->ktype);
  
  bool found = false;
  var node = m->root;
  while (node isnt NULL) {
    int c = cmp(Map_Key(m, node), key);
    if (c is 0) { found = true; break; }
    node = c < 0 ? *Map_Left(m, node) : *Map_Right(m, node);
  }
  
  if (not found) {
    throw(KeyError, "Key %$ not in Map!", key);
    return;
  }
  
  destruct(Map_Key(m, node));
  destruct(Map_Val(m, node));
  
  if ((*Map_Left(m, node) isnt NULL) 
  and (*Map_Right(m, node) isnt NULL)) {
    var pred = Map_Maximum(m, *Map_Left(m, node));
    bool ncol = Map_Get_Color(m, node);
    memcpy((char*)node + 3 * sizeof(var), (char*)pred + 3 * sizeof(var),
      sizeof(struct CelloHeader) + m->ksize +
      sizeof(struct CelloHeader) + m->vsize);
    Map_Set_Color(m, node, ncol);
    node = pred;
  }

  var chld = *Map_Right(m, node) is NULL 
    ? *Map_Left(m, node) 
    : *Map_Right(m, node);
    
  if (Map_Is_Black(m, node)) {
    Map_Set_Color(m, node, Map_Get_Color(m, chld));
    Map_Rem_Fix(m, node);
  }
    
  Map_Replace(m, node, chld);
  
  if ((Map_Get_Parent(m, node) is NULL) and (chld isnt NULL)) {
    Map_Set_Black(m, chld);
  }
  
  m->nitems--;
  free(node);
  
}

static var Map_Iter_Init(var self) {
  struct Map* m = self;
  if (m->nitems is 0) { return NULL; }
  var node = m->root;
  while (*Map_Left(m, node) isnt NULL) {
    node = *Map_Left(m, node);
  }
  return Map_Key(m, node);
}

static var Map_Iter_Next(var self, var curr) {
  struct Map* m = self;
  
  var node = (char*)curr - sizeof(struct CelloHeader) - 3 * sizeof(var);
  var prnt = Map_Get_Parent(m, node);
  
  if (*Map_Right(m, node) isnt NULL) {
    node = *Map_Right(m, node);
    while (*Map_Left(m, node) isnt NULL) {
      node = *Map_Left(m, node);
    }
    return Map_Key(m, node);
  }
  
  while (true) {
    if (prnt is NULL) { return NULL; }
    if (node is *Map_Left(m, prnt)) { return Map_Key(m, prnt); }
    if (node is *Map_Right(m, prnt)) {
      prnt = Map_Get_Parent(m, prnt);
      node = Map_Get_Parent(m, node);
    }
  }
  
  return NULL;
}

static int Map_Show(var self, var output, int pos) {
  struct Map* m = self;
  
  pos = print_to(output, pos, "<'Map' At 0x%p {", self);
  
  var curr = Map_Iter_Init(self);
  
  while (curr isnt NULL) {
    var node = (char*)curr - sizeof(struct CelloHeader) - 3 * sizeof(var);
    pos = print_to(output, pos, "%$:%$",
      Map_Key(m, node), Map_Val(m, node));
    curr = Map_Iter_Next(self, curr);
    if (curr isnt NULL) { pos = print_to(output, pos, ", "); }
  }
  
  pos = print_to(output, pos, "}>");
  
  return pos;
}

static void Map_Traverse(var self, var func) {  
  struct Map* m = self;

  var curr = Map_Iter_Init(self);
  
  while (curr isnt NULL) {
    var node = (char*)curr - sizeof(struct CelloHeader) - 3 * sizeof(var);
    call_with(func, Map_Key(m, node));
    call_with(func, Map_Val(m, node));
    curr = Map_Iter_Next(self, curr);
  }
  
}

var Map = Cello(Map,
  Instance(Doc,
    Map_Name, Map_Brief, Map_Description,
    Map_Examples, Map_Methods),
  Instance(New,      Map_New, Map_Del),
  Instance(Subtype,  Map_Key_Subtype, Map_Key_Subtype, Map_Val_Subtype),
  Instance(Assign,   Map_Assign),
  Instance(Copy,     Map_Copy),
  Instance(Traverse, Map_Traverse),
  Instance(Eq,       Map_Eq),
  Instance(Len,      Map_Len),
  Instance(Get,      Map_Get, Map_Set, Map_Mem, Map_Rem),
  Instance(Clear,    Map_Clear),
  Instance(Iter,     Map_Iter_Init, Map_Iter_Next),
  Instance(Show,     Map_Show, NULL));


