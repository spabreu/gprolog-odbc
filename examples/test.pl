x_connect :- odbc_connect(coolpix, CH), g_assign(ch, CH).

x_get(R) :- x_get('select * from pix', R).
x_get(S,R) :-
	g_read(ch, CH),
	odbc_alloc_stmt(CH, SH),
	odbc_exec_direct(SH, S),
	odbc_fetch(SH),
	odbc_get_data(SH, 1, 4, NUMBER),
	odbc_get_data(SH, 2, 12, FILE),
	odbc_get_data(SH, 3, 12, CAM),
	odbc_get_data(SH, 4, 12, MET),
	odbc_get_data(SH, 5, 12, MODE),
	odbc_get_data(SH, 6, 6, SHUTTER),
	odbc_get_data(SH, 7, 6, AP),
	odbc_get_data(SH, 8, 6, EXP),
	odbc_get_data(SH, 9, 6, FOCAL),
	odbc_get_data(SH, 10, 6, MULT),
	odbc_get_data(SH, 11, 12, ADJUST),
	odbc_get_data(SH, 12, 12, SENS),
	odbc_get_data(SH, 13, 12, WHITE),
	odbc_get_data(SH, 14, 12, SHARP),
	odbc_get_data(SH, 15, 11, DATE),
	odbc_get_data(SH, 16, 12, QUAL),
	R=[NUMBER,FILE,CAM,MET,MODE,SHUTTER,AP,EXP,FOCAL,MULT,ADJUST,SENS,WHITE,SHARP,DATE,QUAL].

x_get2(R1, R2) :- x_get(R1), !, x_get(R2).

olá :- write(olá), nl.
