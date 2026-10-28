# Rebuild the brobdicube data, graph and tables of the paper.
#
#   make            build brobdicube, generate data, graph and tables
#   make data       only the brobdicube runs (resumable; about 1 hour on 16 cores)
#   make others     sortnet96, rcube1d, rcube2d and permnet72 runs (resumable)
#   make bounds     worst-case bounds of those four solvers (instant)
#   make paper PAPER_SRC=/path/to/paper/sources PAPER_OUT=/path/to/output
#                   copy the paper sources to PAPER_OUT (outside this repo;
#                   default ../paper) with the regenerated graph and tables

PYTHON    ?= python3
JOBS      ?= $(shell echo $$(( $$(sysctl -n hw.ncpu 2>/dev/null || nproc) - 2 )))
MEM_GB    ?= 40
PAPER_SRC ?= ../zipcontents
PAPER_OUT ?= ../paper

.PHONY: all build data others bounds model worst model-p1 worst-p1 sim figures tables check paper legacy clean-data

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

# Redraw the published graph and tables from olddata/ (permnet62 instead of
# the new brobdicube data) and diff the tables against the paper; then list
# how the new tables differ from the published ones.
check:
	$(PYTHON) scripts/make_graph.py --brob legacy --out figures/check --scale lgn --ymax 300 --no-centers
	$(PYTHON) scripts/make_tables.py --brob legacy --out tables/check
	@if [ -f $(PAPER_SRC)/main.tex ]; then \
	  echo "--- published tables vs. olddata (should match)"; \
	  $(PYTHON) scripts/compare_tables.py $(PAPER_SRC)/main.tex tables/check; \
	  echo "--- published tables vs. new data"; \
	  $(PYTHON) scripts/compare_tables.py $(PAPER_SRC)/main.tex tables; \
	fi

paper: figures tables
	$(PYTHON) scripts/update_paper.py $(PAPER_SRC) $(PAPER_OUT) --label permnet61

# Re-import the previously collected data (already committed in data/legacy).
legacy:
	$(PYTHON) scripts/import_olddata.py olddata data/legacy

clean-data:
	rm -rf data/brobdicube
