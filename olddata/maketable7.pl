my @header = qw(n) ;
my @names = () ;
for $f (glob "arena/*.txt") {
   $name = $f ;
   $name =~ s/.txt// ;
   $name =~ s,arena/,, ;
   push @header, $name ;
   push @names, $name ;
   open F, $f or die "Can't read $f\n" ;
   while (<F>) {
      chomp ;
      @f = split " ", $_ ;
      $n = $f[0] ;
      $w = $f[1] ;
      $bestw = $f[2] ;
      $moves = $f[3] ;
      $mp{"$n $name"} = $moves ;
      $vals{$n}++ ;
   }
   close F ;
}
for $f ((glob "bounds/*.txt"), (glob "notbounds/*.txt")) {
   $name = $f ;
   $name =~ s/.txt// ;
   $name =~ s,.*bounds/,, ;
   push @header, $name ;
   push @names, $name ;
   open F, $f or die "Can't read $f\n" ;
   while (<F>) {
      chomp ;
      @f = split " ", $_ ;
      $n = $f[0] ;
      $moves = $f[1] ;
      $mp{"$n $name"} = $moves ;
      $vals{$n}++ ;
   }
   close F ;
}
@vals = sort { $a <=> $b } keys %vals ;
my @fields = qw(lb edgewc orbwc sp1dwc sp2dwc sort96wc permwc brob4wc just62wc rcube1d rcube2d sort96 permnet brob4 just62) ;
my @toshow = qw(4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304 8388608 16777216 33554432 67108864 134217728 268435456 536870912) ;
sub getval {
   my $n = shift ;
   my $name = shift ;
   die "Missing $n $name\n" if !defined($mp{"$n $name"}) ;
   my $t = $mp{"$n $name"} ;
   if ($t =~ /\+/) {
      $t = int($t) ;
   }
   return $t / (6 * ($n-2) * ($n-2)) ;
}
for $n (@toshow) {
   print "$n:\n" ;
   for ($i=0; $i<@fields; $i++) {
      $v = getval($n, $fields[$i]) ;
      printf(" %13s: %s\n", $fields[$i], $v) ;
   }
}
