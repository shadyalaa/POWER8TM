## Framework with a variety of Transactional Memory implementations and benchmarks ##

**Requirements:**

* Install: tcmalloc (needed for all backends)
* Install: boost libraries (needed for TPCC)


**Backends:**

* p8tm: POWER8-TM with time efficient read-set tracking

* p8tm-se: POWER8-TM with space efficient read-set tracking

* p8tm-sepp: POWER8-TM with aggressive space efficient read-set tracking

* p8tm-ucb: POWER8-TM with time efficient read-set tracking and self-tuning module enabled

* herwl: Hardware Read-Write Lock Elision

* htm-sgl: Power8 HTM used with a single global lock fallback

* norec: NoReC STM

* hybrid-norec: Hybird NoRec 

**Benchmarks:**

* Concurrent data-structures (hashmap)
* STAMP suite
* TPCC on in-memory database


To compile the benchmarks, use there is a specialized script under each benchmarks folder. 
These scripts expect three parameters (in this order):

1. the backend name, corresponding to the folder under "backends"

2. the number of retries for HTM usage, which may be omitted (default value 5)

3. the number of retries for ROT usage after falling back from normal HTM (default value 2), must be >= 1 
   or in case of htm-sgl or hyrbid-norec: the retry policy to adopt in case of HTM persistent aborts, i.e., how the number of retries is updated upon a 
   persistent abort, which may be omitted (possible values: 0, do not retry in htm and resort to the fallback path; 1, decrease by
   one; 2, divide by two; default value 1)

Hence, a common usage shall be: "bash build-datastructures.sh p8tm 10 5"


