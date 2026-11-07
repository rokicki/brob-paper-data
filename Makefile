# Rebuild the brobdicube data, graph and tables of the paper.
#
#   make            build brobdicube, generate data, graph and tables
#   make data       only the brobdicube runs (resumable; about 1 hour on 16 cores)
#   make others     sortnet96, rcube1d, rcube2d and permnet72 runs (resumable)
#   make bounds     worst-case bounds of those four solvers (instant)
#   make figures    the graphs;  make tables  the table fragments and CSVs
#   make check      rerun brobdicube's planner and a sample of the stored
#                   simulation runs, and check they reproduce exactly
#   make paper PAPER_SRC=/path/to/paper/sources PAPER_OUT=/path/to/output
#                   copy the paper sources to PAPER_OUT (outside this repo;
#                   default ../paper) with the regenerated graph and tables

PYTHON    ?= python3
JOBS      ?= $(shell echo $$(( $$(sysctl -n hw.ncpu 2>/dev/null || nproc) - 2 )))
MEM_GB    ?= 40
PAPER_SRC ?= ../zipcontents
PAPER_OUT ?= ../paper

.PHONY: all build data others bounds model worst model-p1 worst-p1 sim figures tables check paper clean-data

all: build data others bounds figures tables check

build:
	$(MAKE) -C brobdicube brobdicube
	$(MAKE) -C sortnet96 solve96
	$(MAKE) -C permnet72 brobdicube

data: build
	$(PYTHON) scripts/run_brobdicube.py --jobs $(JOBS) --mem-gb $(MEM_GB)

others: build
	$(PYTHON) scripts/run_others.py --jobs $(JOBS) --mem-gb $(MEM_GB)

bounds:
	$(PYTHON) scripts/bounds.py

model worst model-p1 worst-p1 sim: build
	$(PYTHON) scripts/run_brobdicube.py --jobs $(JOBS) --mem-gb $(MEM_GB) $@ summary

figures:
	$(PYTHON) scripts/make_graph.py
	$(PYTHON) scripts/make_graph.py --scale all

tables:
	$(PYTHON) scripts/make_tables.py
	$(PYTHON) scripts/decompositions.py

check: build
	$(PYTHON) scripts/check.py --jobs $(JOBS)

paper: figures tables
	$(PYTHON) scripts/update_paper.py $(PAPER_SRC) $(PAPER_OUT) --label brobdicube

clean-data:
	rm -rf data/brobdicube
