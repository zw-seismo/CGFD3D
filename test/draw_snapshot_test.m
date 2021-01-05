output_dir='export/home/wangyh/CGFD3D-elastic/project/output'
%output_dir='/home/zhangw/work/cgfd_mpi/output.6.free'
%output_dir='/home/zhangw/work/cgfd_mpi/output.3.1src.discount'

nprocx = 3;
nprocy = 3;

%figure

snap_name = 'volume_vel'
i_indx = 49;
vname = 'Vy'

figure;
snap_fname = {};

  for ipx = 1 : nprocx
  for ipy = 1 : nprocy
    snap_fname{ipx, ipy} = [output_dir, '/', ...
        snap_name, ...
        '_px', num2str(ipx-1), '_py' num2str(ipy-1), ...
        '.nc'];
  end
  end
  
  %%
  tmpx = [];
  var = [];
  
for ipx = 1 : nprocx
        tmpy = [];
for ipy = 1 : nprocy      
        tmp = ncread(snap_fname{ipx ,ipy}, vname );
        tmpy = horzcat(tmpy, tmp);
end
        tmpx = vertcat(tmpx, tmpy);
end

%%
var = tmpx;
%   var = nc_varget(snap_fname, vname);

%%mesh grid
[X,Y,Z,T] = size(var);
dx = 1; dy = 1; dz = 5; dt  = 1;

[x,y,z] = meshgrid(0:dx:(X-1)*dx, 0:dy:(Y-1)*dy, 0:dz:(Z-1)*dz);
xslice = [60];
yslice = [99];%1:30:100;
zslice = [10];

%for it = 1:10:1500
for it = 1:10:500
  %figure
%   pcolor(squeeze(var(60,:,:,it)));
  slice(x,y,z,var(:,:,:,it),xslice, yslice, zslice);
  shading interp;
%   axis([0 xslice 0 yslice zslice max(z(1,1,:)) ]); 
  %pcolor(squeeze(var(:,23,:)));
  %title(['it=',num2str(it)]);
  title(['it=',num2str(it), ' of ', output_dir]);
  %caxis([-1,1]*1e-8);
  caxis([-1,1]*1e-10);
  %caxis([-1,1]*1e-12);
  colorbar
  drawnow;
%   pause(0.5);

  %figure;
  %%plot(squeeze(var(59,23,:)),'-*')
  %plot(squeeze(var(63,23,:)),'-*')
  %%plot(squeeze(var(63,:,43)),'-*')

  %figure
  %plot(squeeze(var10(63,23,:)),'-*')
  %figure
  %plot(squeeze(var00(63,23,:)),'-*')
end

