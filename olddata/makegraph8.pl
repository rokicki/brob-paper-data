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
for $f (glob "bounds/*.txt") {
   next if $f =~ /edge/ || $f =~ /qqwref/ || $f =~ /lbfull/ ;
   $name = $f ;
   $name =~ s/.txt// ;
   $name =~ s,bounds/,, ;
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
#print "n,lb,orbwc,spwc,sortwc,s2dwc,s74wc,s1av,s2dav,sortav,s84av,s74\n" ;
print join ",", @header ;
print "\n"  ;
for $n (@vals) {
   next if $vals{$n} != 14 ;
   print "$n" ;
   for ($i=0; $i<@names; $i++) {
      $mv = $mp{"$n $names[$i]"} ;
      $mv = $mv / (($n-2) * ($n-2) * 6) ;
      print ",$mv" ;
   }
   print "\n" ;
}
