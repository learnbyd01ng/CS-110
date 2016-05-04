# CS110 Assignment 4 Makefile

PROGS = aggregate tptest
CXX = g++

NA_LIB_SRC = news-aggregator.cc \
	     news-aggregator-liberal.cc \
	     news-aggregator-conservative.cc \
	     news-aggregator-log.cc \
	     news-aggregator-utils.cc \
	     stream-tokenizer.cc \
	     rss-feed.cc \
	     rss-feed-list.cc \
	     html-document.cc \
	     rss-index.cc 

TP_LIB_SRC = thread-pool.cc

WARNINGS = -Wall -pedantic
DEPS = -MMD -MF $(@:.o=.d)
DEFINES = -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD
INCLUDES = -I/usr/class/cs110/local/include -I/usr/include/libxml2

CXXFLAGS = -g $(WARNINGS) -O0 -std=c++0x $(DEFINES) $(INCLUDES)
LDFLAGS = -lm -lrand -lxml2 -L/usr/class/cs110/local/lib -lthreads -pthread

NA_LIB_OBJ = $(patsubst %.cc,%.o,$(patsubst %.S,%.o,$(NA_LIB_SRC)))
NA_LIB_DEP = $(patsubst %.o,%.d,$(NA_LIB_OBJ))
NA_LIB = news-aggregator.a

TP_LIB_OBJ = $(patsubst %.cc,%.o,$(patsubst %.S,%.o,$(TP_LIB_SRC)))
TP_LIB_DEP = $(patsubst %.o,%.d,$(TP_LIB_OBJ))
TP_LIB = thread-pool.a

PROGS_SRC = $(PROGS:%=%.cc)
PROGS_OBJ = $(patsubst %.cc,%.o,$(patsubst %.S,%.o,$(PROGS_SRC)))
PROGS_DEP = $(patsubst %.o,%.d,$(PROGS_OBJ))

TMP_PATH := /usr/bin:$(PATH)
export PATH = $(TMP_PATH)

default: $(PROGS)

$(PROGS): %:%.o $(NA_LIB) $(TP_LIB)
	$(CXX) $^ $(LDFLAGS) -o $@

# The soln target makes solution versions of the program.
# For each program 'binky' in $(PROGS) the rule specifies how
# to build 'binky_soln' by linking the binky.c code to the
# sample library versions of ThreadPool, producing a binky_soln
# you can compare to binky
SOLN_PROGS = $(PROGS:%=%_soln)
soln: $(SOLN_PROGS)

$(SOLN_PROGS): %_soln:%.o $(NA_LIB) /usr/class/cs110/samples/assign4/libthread-pool_soln.a
	$(CXX) $^ $(LDFLAGS) -o $@

$(NA_LIB): $(NA_LIB_OBJ)
	rm -f $@
	ar r $@ $^
	ranlib $@

$(TP_LIB): $(TP_LIB_OBJ)
	rm -f $@
	ar r $@ $^
	ranlib $@

clean::
	rm -f $(PROGS) $(SOLN_PROGS)
	rm -f $(PROGS_OBJ) $(PROGS_DEP)
	rm -f $(NA_LIB) $(NA_LIB_DEP) $(NA_LIB_OBJ)
	rm -f $(TP_LIB) $(TP_LIB_DEP) $(TP_LIB_OBJ)

spartan:: clean
	\rm -fr *~

.PHONY: all clean spartan soln

-include $(NA_LIB_DEP) $(TP_LIB_DEP) $(PROGS_DEP)
