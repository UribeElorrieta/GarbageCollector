#include "gc_details.h"
#include "gc_iterator.h"
#include <cstdlib>
#include <iostream>
#include <list>
#include <typeinfo>
/*
    Pointer implements a pointer type that uses
    garbage collection to release unused memory.
    A Pointer must only be used to point to memory
    that was dynamically allocated using new.
    When used to refer to an allocated array,
    specify the array size.
*/
template <class T, int size = 0> class Pointer {
private:
  // refContainer maintains the garbage collection list.
  static std::list<PtrDetails<T>> refContainer;
  // addr points to the allocated memory to which
  // this Pointer pointer currently points.
  T *addr;
  /*  isArray is true if this Pointer points
      to an allocated array. It is false
      otherwise.
  */
  bool isArray;
  // true if pointing to array
  // If this Pointer is pointing to an allocated
  // array, then arraySize contains its size.
  unsigned arraySize; // size of the array
  static bool first;  // true when first Pointer is created
  // Return an iterator to pointer details in refContainer.
  typename std::list<PtrDetails<T>>::iterator findPtrInfo(T *ptr);

public:
  // Define an iterator type for Pointer<T>.
  typedef Iter<T> GCiterator;
  // Empty constructor
  // NOTE: templates aren't able to have prototypes with default arguments
  // this is why constructor is designed like this:
  Pointer() { Pointer(NULL); }
  Pointer(T *);
  // Copy constructor.
  Pointer(const Pointer &);
  // Destructor for Pointer.
  ~Pointer();
  // Collect garbage. Returns true if at least
  // one object was freed.
  static bool collect();
  // Overload assignment of pointer to Pointer.
  T *operator=(T *t);
  // Overload assignment of Pointer to Pointer.
  Pointer &operator=(Pointer &rv);
  // Return a reference to the object pointed
  // to by this Pointer.
  T &operator*() { return *addr; }
  // Return the address being pointed to.
  T *operator->() { return addr; }
  // Return a reference to the object at the
  // index specified by i.
  T &operator[](int i) { return addr[i]; }
  // Conversion function to T *.
  operator T *() { return addr; }
  // Return an Iter to the start of the allocated memory.

  Iter<T> begin() {
    int _size;
    if (isArray)
      _size = arraySize;
    else
      _size = 1;
    return Iter<T>(addr, addr, addr + _size);
  }

  // Return an Iter to one past the end of an allocated array.
  Iter<T> end() {
    int _size;
    if (isArray)
      _size = arraySize;
    else
      _size = 1;
    return Iter<T>(addr + _size, addr, addr + _size);
  }
  // Return the size of refContainer for this type of Pointer.
  static int refContainerSize() { return refContainer.size(); }
  // A utility function that displays refContainer.
  static void showlist();
  // Clear refContainer when program exits.
  static void shutdown();
};

// STATIC MEMBER INITIALIZATION.
// Creates storage for the static variables
// Initializes both refContainer (list of PtrDetails objects) and
// first (indicates whether it is the first pointer to be collected).
template <class T, int size>
std::list<PtrDetails<T>> Pointer<T, size>::refContainer;
// By default first is true, meaning that at the very beginning of the
// instantiation, there is no memory block being pointed at.
template <class T, int size>
bool Pointer<T, size>::first = true;

// INSTANCES MEMBER INITIALIZATION.

////////////////////////////////////////////////////////////////////////////
//                         POINTER CONSTRUCTOR                            //
////////////////////////////////////////////////////////////////////////////
template <class T, int size>
Pointer<T, size>::Pointer(T * t) {
  // Register shutdown() as an exit function.
  if (first) {
    // This function lets calling "shutdown" function when execution thread
    // returns from main(). Just set when at least one object has been referenced
    // for garbage collection.
    atexit(shutdown);
  }
  // Reset first static member.
  first = false;
  // First we should know if the address pointed at by the given pointer (t),
  // is already pointed at by other pointer(s) in the list of PtrDetails items.
  // To do that, we need to create an iterator to the list of PtrDetails items.
  typename std::list<PtrDetails<T>>::iterator p;
  // We call the function findPtrInfo which tells us if there is a pointer
  // that is pointing at to the given address (t).
  p = findPtrInfo(t);
  // We check if the iterator is pointing at any item.
  if (p != refContainer.end()) {
    // In case it exists a PtrDetails object, we update it's counter.
    p->upRefCount();
  }
  else {
    // In case is a pointer to a new allocated item in the heap.
    // Include that item in the container for references.
    refContainer.emplace_back(t, size);
  }
  // But in any case, the values have to be stored within this pointer object.
  addr = t;
  // Store whether it is an array or not (size is greater than 0)
  if (size > 0) {
    isArray = true;
  }
  else {
    isArray = false;
  }
  arraySize = size;
}

////////////////////////////////////////////////////////////////////////////
//                       POINTER COPY CONSTRUCTOR                         //
////////////////////////////////////////////////////////////////////////////

template <class T, int size>
Pointer<T, size>::Pointer(const Pointer &ob) {
    typename std::list<PtrDetails<T>>::iterator p;
    // A copy constructor copies the given object content to a new object,
    // so a PtrDetails object must exist.
    p = findPtrInfo(ob.addr);
    // First, update the reference count for that memory block.
    p->upRefCount();
    // Then we copy all the member instance info.
    addr = ob.addr;
    isArray = ob.isArray;
    arraySize = ob.arraySize;
}

////////////////////////////////////////////////////////////////////////////
//                       POINTER DESTRUCTOR                              //
////////////////////////////////////////////////////////////////////////////
template <class T, int size>
Pointer<T, size>::~Pointer() {
  typename std::list<PtrDetails<T> >::iterator p;
  // A PtrDetails item should be found at the reference container.
  p = findPtrInfo(addr);
  // We decrease the reference count for this address PtrDetails.
  p->downRefCount();
  // Collect garbage when a pointer goes at of scope.
  std::cout << "Before collecting garbage\n";
  showlist();

  collect();
  // If a less frequent calls to garbage collection needed, 
  // revise this piece of code.

  std::cout << "After collecting garbage\n";
  showlist();

}

////////////////////////////////////////////////////////////////////////////
//                          COLLECT GARBAGE                               //
////////////////////////////////////////////////////////////////////////////
// Returns true if at least one object was freed.
template <class T, int size>
bool Pointer<T, size>::collect() {
  bool memfreed = false;
  typename std::list<PtrDetails<T> >::iterator p;
  p = refContainer.begin();

  while(p != refContainer.end()) {
    // Scan refContainer looking for unreferenced pointers.
    if (p->zeroRefCount()) {
      // Means there are no references to this address, so we should
      // delete the memory block.
      // Free memory for that address that is no more pointed at.
      if (p->isArray) {
        delete[] p->memPtr;
      }
      else {
        delete p->memPtr;
      }
      // Tell we have freed memory.
      memfreed = true;
      // Remove unused entry from refContainer.
      p = refContainer.erase(p);
    }
    else {
      // In case the item have not been erased, update the iterator
      // to go through the rest of the container of references.
      p++;
    }
  }
  // Returns whether the memory has been freed or not.
  return memfreed;
}

////////////////////////////////////////////////////////////////////////////
//                   pointer TO POINTER ASSIGNMENT                        //
////////////////////////////////////////////////////////////////////////////
template <class T, int size>
T * Pointer<T, size>::operator=(T *t) {
   // Check whether it is a PtrDetails object for this address in the 
  // references container.
  typename std::list<PtrDetails<T>>::iterator p;
  // The object that this pointer is pointing at may exist in the
  // references container.
  p = findPtrInfo(addr);
  if (p != refContainer.end()) {
    // In this case, the assignment of a new address forces the previous one
    // lose it's pointer, so it's reference count should be decreased.
    p->downRefCount();
  }
  // Now we need to check whether the new address to which is going
  // to point at has been previously being appointed.
  p = findPtrInfo(t);
  if (p == refContainer.end()) {
    // There is no such object, so we need to create a new one
    // and emplace it back.
    refContainer.emplace_back(t, size);
  }
  else {
    // In case it exist, we should increment the counter for this reference.
    p->upRefCount();
  }
  // Update the Pointer with the given pointer.
  addr = t;
  arraySize = size;
  isArray = size > 0;

  // Assign the same pointer.
  return t;
}

////////////////////////////////////////////////////////////////////////////
//                   POINTER TO POINTER ASSIGNMENT                        //
////////////////////////////////////////////////////////////////////////////
template <class T, int size>
Pointer<T, size> &Pointer<T, size>::operator=(Pointer &rv) {
  // Avoid self-assignments.
  if (* this!=rv) {
    // As there is going to be a new pointer to the address pointing at by
    // the given parameter t, the PtrDetails object must be updated.
    typename std::list<PtrDetails<T>>::iterator p;
    // The object should exist in the references container.
    p = findPtrInfo(addr);
    // First, update the reference count for that memory block.
    p->downRefCount();
    // Of course, the address has been referenced before, so exists in the 
    // references container.
    p = findPtrInfo(rv.addr);
    // Update the reference count for that memory block.
    p->upRefCount();
    // Then we copy all the member instance info.
    addr = rv.addr;
    isArray = rv.isArray;
    arraySize = rv.arraySize;
  }
  // Return the address to the current Pointer object that has assigned
  // the content of the given rv parameter.
  return *this;
}

// A utility function that displays refContainer.
template <class T, int size> void Pointer<T, size>::showlist() {
  typename std::list<PtrDetails<T>>::iterator p;
  std::cout << "refContainer<" << typeid(T).name() << ", " << size << ">:\n";
  std::cout << "memPtr refcount value\n ";
  if (refContainer.begin() == refContainer.end()) {
    std::cout << " Container is empty!\n\n ";
  }
  for (p = refContainer.begin(); p != refContainer.end(); p++) {
    std::cout << "[" << (void *)p->memPtr << "]"
              << " " << p->refCount << " ";
    if (p->memPtr)
      std::cout << " " << *p->memPtr;
    else
      std::cout << "---";
    std::cout << std::endl;
  }
  std::cout << std::endl;
}
// Find a pointer in refContainer.
template <class T, int size>
typename std::list<PtrDetails<T>>::iterator
Pointer<T, size>::findPtrInfo(T *ptr) {
  typename std::list<PtrDetails<T>>::iterator p;
  // Find ptr in refContainer.
  for (p = refContainer.begin(); p != refContainer.end(); p++)
    if (p->memPtr == ptr)
      return p;
  return p;
}
// Clear refContainer when program exits.
template <class T, int size> void Pointer<T, size>::shutdown() {
  if (refContainerSize() == 0)
    return; // list is empty
  typename std::list<PtrDetails<T>>::iterator p;
  for (p = refContainer.begin(); p != refContainer.end(); p++) {
    // Set all reference counts to zero
    p->refCount = 0;
  }
  collect();
}