#!/usr/bin/perl
# /*  ArchC Library for tools generated by the ArchC Pre-processor
#     Copyright (C) 2002-2004  The ArchC Team

#     This library is free software; you can redistribute it and/or
#     modify it under the terms of the GNU Lesser General Public
#     License as published by the Free Software Foundation; either
#     version 2.1 of the License, or (at your option) any later version.

#     This library is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#     Lesser General Public License for more details.
# */
# /********************************************************/
# /* ArchC Loader                                         */
# /* Author:  Sandro Rigo                                 */
# /*                                                      */
# /*                                                      */
# /* The ArchC Team                                       */
# /* Computer Systems Laboratory (LSC)                    */
# /* IC-UNICAMP                                           */
# /* http://www.lsc.ic.unicamp.br                         */
# /********************************************************/



($file) = @ARGV;

print( stderr "\n\nArchC: Loading binary file: ", $file, "\n" );

#Testing input file
if ( ! open(INPUT, $file) ) { 
		&error("Application Loader could not open input file"); 
} 
close(INPUT);

#Opening temporary files
if ( ! open(OUTPUT,">loader.ac") ) { 
		&error("Could not open output file"); 
} 

if ( ! open(OUTPUT2,">loader.tmp") ) { 
		&error("Could not open output file"); 
} 


#Calling objdump to generate the temporary hexa files
if ( ! open(TEMP, "objdump -s $file 2>/dev/null|" )) { 
		&error("Could not open temporary output file"); 
} 

if ( ! open(TEMP2, "objdump -x $file 2>/dev/null|" )) { 
		&error("Could not open temporary output file"); 
} 

###############################
## Extracting Memory Size    ##
## This is necessary for heap##
## initialization in ArchC   ##
###############################
while( <TEMP2> ){

		@words = split(' ', $_);
		
		if( $words[2] eq 'memsz' ){
				print ( OUTPUT2 $words[3]);
		}
}
close(TEMP2);
close(OUTPUT2);

###############################
## Processing Temporary File ##
###############################
#Ignoring first line 
$line = <TEMP>; 

#Processing  INPUT file
while( <TEMP> ) {
		@words = split(' ', $_);
		

		#Ignoring unuseful sections
 		if( ($words[0] eq 'Contents') ){
				
				$load_data = 0;
				

				if( ( $words[3] eq '.text:') ||
						( $words[3] eq '.data:' ) || 
						( $words[3] eq '.sdata:' ) ||
						( $words[3] eq '.lit8:' ) ||
						( $words[3] eq '.rodata:' )){
						$load_data = 1;
						print( OUTPUT $words[3], "\n" );
						next
				}

		}

		if( ($load_data == 1) ){
				
			  pop(@words);  #Eliminating last collumn
 				print( OUTPUT join(" ",@words[0,1,2,3,4] ), 
							 "\n"); 
		}

}

close(OUTPUT);
close(TEMP);

# define an error routine: 
sub error { 
		($message) = @_; 
		print("ArchC ERROR: ", 
					$message, "\n"); 
					
		exit(0); 
} 