function [gd,fnorm]=sgrpdlay(x,fnorm);
%SGRPDLAY Group delay estimation of a signal.
%	[GD,FNORM]=SGRPDLAY(X,FNORM) estimates the group delay of
%	signal X at the normalized frequency(ies) FNORM.
%
%	X     : signal in the time-domain.
%	FNORM : normalized frequency. By default, FNORM is a 
%               linearly spaced vector between -0.5 and 0.5 with
%               length(X) elements.
%	GD    : computed group delay. When GD equals zero, it means that
%	 	the estimation of the group delay for this frequency was 
%		outside the interval [1 xrow], and therefore meaningless.
%
%	Example : 
%	 N=128; x=amgauss(N,64,30).*fmlin(N,0.1,0.4);
%	 fnorm=0.1:0.04:0.38; gd=sgrpdlay(x,fnorm); 
%	 t=2:N-1; instf=instfreq(x,t);
%	 plot(t,instf,gd,fnorm); axis([1 N 0 0.5]); 

%	F. Auger, March 1994, July 1995.
%	Copyright (c) 1996 by CNRS (France).
%
%	------------------- CONFIDENTIAL PROGRAM -------------------- 
%	This program can not be used without the authorization of its
%	author(s). For any comment or bug report, please send e-mail to 
%	f.auger@ieee.org 

if (nargin == 0),
 error('At least one parameter required');
end;

[xrow,xcol] = size(x); 
if (xcol~=1),
 error('x must have only one column');
end;
Ex=mean(abs(x).^2); 
Threshold=1.0e-6;

if (nargin==1), 
 Num=fft(x .* (1:xrow)');
 Den=fft(x);
 ratio=(real(Num./Den) .* (real(Num./Den)>=1) .* (real(Num./Den)<=xrow+3));
 gd=fftshift(ratio);
 if (nargout==2),
  if (rem(xrow,2)==0),
   L=xrow/2;
   fnorm=(-L:L-1)/xrow;
  else
   L=(xrow-1)/2;
   fnorm=(-L:L)/xrow;
  end;
 end;
elseif (nargin==2),
 [fnormrow,fnormcol] = size(fnorm);
 if (fnormrow~=1),
  error('FNORM must have only one row');
 end;
 expo=exp(-j*2.0*pi*fnorm'*(0:xrow-1));
 Num=expo*(x .* (1:xrow)');
 Den=expo*x; 
 gd=(real(Num./Den) .* (real(Num./Den)>=1) .* (real(Num./Den)<=xrow+3));
end;

