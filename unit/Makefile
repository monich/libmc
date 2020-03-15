# -*- Mode: makefile-gmake -*-

all:
%:
	@$(MAKE) -C test_mecard $*
	@$(MAKE) -C test_record $*

clean: unitclean
	rm -f coverage/*.gcov
	rm -fr coverage/report
