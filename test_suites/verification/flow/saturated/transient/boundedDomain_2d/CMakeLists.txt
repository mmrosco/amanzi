# -*- mode: cmake -*-

project (BOUNDEDDOMAIN_2D)

add_amanzi_comparison_test(boundedDomain_2d
                           INPUT amanzi_boundedDomain_2d.xml
                           ABSOLUTE_TOLERANCE 1e-8
                           RELATIVE_TOLERANCE 1e-4
                           OBSERVATION drawdown
                           REFERENCE ${BOUNDEDDOMAIN_2D_SOURCE_DIR}/golden_output/observations.out
                           NPROCS 4)
