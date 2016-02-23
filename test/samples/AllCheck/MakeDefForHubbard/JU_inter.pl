#!/usr/local/bin/perl
  #input!!
  &input;
  #input!!
  $orb_num=$tmp_orb;
  $L_x=$tmp_Lx;
  $L_y=$tmp_Ly;
  $L=$L_x*$L_y;
  $Ns=$L;
  $All_N=$Ns*$orb_num;
  printf("CHECK U_inter L_x=$L_x L_y=$L_y J=$J orb=$orb_num All_N=$All_N \n");

  $tmp=$L;
  $fname="zcoulombinter.def";
  open(FILE,">$fname");
  printf FILE "========misawa======== \n";
  printf FILE "NCoulombIntra $tmp  \n";
  printf FILE "========misawa======== \n";
  printf FILE "========CoulombInter ====== \n";
  printf FILE "========misawa======== \n";

  $J_V = -0.25*$J;
  for($site_i=0;$site_i<$Ns;$site_i+=1){
    $orb_i  = 0;
    $all_i  = $orb_num*$site_i+$orb_i;
    $x      = $site_i%$L_x;
    $y      = ($site_i-$x)/$L_x;
    $orb_j  = 0;

    #(1,0)
    $x_tmp  = ($x+1)%$L_x;
    $y_tmp  = $y;
    $site_j = $x_tmp+$L_x*$y_tmp;
    $all_j  = $orb_num*$site_j+$orb_j;
    printf FILE ("%4d  %4d %lf\n",$all_i,$all_j,$J_V);
  }
 close(FILE);
 printf "U_inter.prl finish \n";
 
 sub input{
  #input START 
  $Lx_cnt=0;
  $Ly_cnt=0;
  $orb_cnt=0;
  $file=sprintf("input.txt");
  open(INPUTFILE,$file);
  while($name=<INPUTFILE>){
    chomp $name;
    #DELETE EMPTY
    $_=$name; 
    s/^\s+//;
    $name=$_; 
    @tmp = split /\s+/, $name;
    #printf "$tmp[0] $tmp[1] \n";
    if($tmp[0] eq 'Lx'){
      #printf "AA $tmp[0] $tmp[1] \n";
      $tmp_Lx = $tmp[1];
      $Lx_cnt=1;
    } 
    if($tmp[0] eq 'Ly'){
      #printf "AA $tmp[0] $tmp[1] \n";
      $tmp_Ly = $tmp[1];
      $Ly_cnt=1;
    } 
    if($tmp[0] eq 'orb_num'){
      #printf "AA $tmp[0] $tmp[1] \n";
      $tmp_orb = $tmp[1];
      $orb_cnt=1;
    } 
    if($tmp[0] eq 'J'){
      #printf "AA $tmp[0] $tmp[1] \n";
      $J = $tmp[1];
    } 
  }
  if($Lx_cnt==0 || $Ly_cnt==0||$orb_cnt==0){
    printf "FAITAL ERROR IN input.txt !!!!!!!!!!!!! \n";
  }
  #input FINISH
 }
