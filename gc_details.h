// This class defines an element that is stored
// in the garbage collection information list.
template <class T> class PtrDetails {
public:
  unsigned refCount;  // current reference count
  T * memPtr;          // pointer to allocated memory
  bool isArray;       // true if pointing to array
  unsigned arraySize; // If memPtr is pointing to an allocated array size of array

  PtrDetails<T>(T * ptr, unsigned size = 0) {
    // Assign the pointer.
    memPtr = ptr;
    // The first time a PtrDetails object is created, there is just
    // one pointer pointing at the address stored within.
    refCount = 1;
    if (size > 0) {
      // Size longer than zero means the pointer is pointing at an array.
      isArray = true;
    }
    else {
      isArray = false;
    }
    // Stores the size.
    arraySize = size;
  } 

  // Copy constructor
  PtrDetails<T>(const PtrDetails &ob) {
    // First, update the reference count for that memory block.
    // The object exists
    // Then we copy all the member instance info.
    memPtr = ob.memPtr;
    isArray = ob.isArray;
    arraySize = ob.arraySize;
  }

  // Just increments the reference count for the address this instance is
  // storing information for.
  void upRefCount() {refCount ++;} 
  // Decreases the same reference count.
  void downRefCount() {refCount --;}
  // Tells whether there ar no references to this address so that it can 
  // be deleted.
  bool zeroRefCount() {return refCount == 0;}
};

// Overloading operator== allows two class objects to be compared (needed by STL list).
template <class T>
bool operator==(const PtrDetails<T> &obj_1,
                const PtrDetails<T> &obj_2)
{
    return (obj_1.memPtr == obj_2.memPtr) && (obj_1.arraySize == obj_2.arraySize);
}