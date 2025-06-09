# Compiler
LL(1) grammar compiler 
	Takes input code, runs through a Lexar analyzer(confirms all characters in the input belong to the specified language).
	Takes an LL(1) grammar table from the language and its rules and uses it to perform syntax analysis on the input code. 
	Code without syntax or Lexar errors is run again to test for semantic errors.
	The input code with no errors is converted to intermediate 3 TAC code(still to be done)
	Later conversion to assembly code is possible but is not planned.
	At each processing step errors are logged and an attempt at error correction is made where possible. 
