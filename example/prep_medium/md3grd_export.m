function md3grd_export(fnm_ou, md)

% This function exports structure model to md3grd file.
%  fnm_ou: string, the output file name;
%  md: model structure. What should be kept in md depends on its media_type:
%     media_type: supported value 
%       one_component, 
%       acoustic_isotropic, 
%       elastic_isotropic, 
%       elastic_vti_prem, elastic_vti_thomsen, elastic_vti_cij,
%       elastic_tti_thomsen, elastic_tti_bond,
%       elastic_aniso_cij
%
%     num_of_intfce: number of interfaces or layers,
%     nx: number of sampling points along x-axis
%     ny: number of sampling points along y-axis
%     x0: x0 of first sampling points along x-axis
%     y0: y0 of first sampling points along y-axis
%     dx: dx of sampling points along x-axis
%     dx: dy of sampling points along y-axis
%     num_of_point_per_lay: [num_of_intfce] array, number of points of each layer
%     elev: {num_of_intfce} cell array, each elem is [nx,ny,num_of_intfce] array
%     
%     the meaning of possible vars ({num_of_intfce}[ny,nx, num_of_point_per_lay] array
%       val: only for one_component, keep target values
%       Vp
%       Vs
%       

%-- create output file
fid = fopen(fnm_ou,'w');

%-- 1st: media_type, value could be:
fprintf(fid, '%s\n',md.media_type);

%-- 2nd: number of layer
fprintf(fid, '%d\n', md.num_of_intfce);

%-- 3rd: number of points of each layer
for n = 1 : md.num_of_intfce
   	fprintf(fid, ' %d', md.num_of_point_per_lay(n)); 
end
fprintf(fid, '\n'); 

%-- 4rd
fprintf(fid, '%d %d %f %f %f %f\n', md.nx, md.ny, md.x0, md.y0, md.dx, md.dy);

%-- rest
  for ilay = 1 : md.num_of_intfce
      disp([num2str(ilay), 'th-layer of total ', num2str(md.num_of_intfce), ' layers'])
      for k = 1 : md.num_of_point_per_lay(ilay)
      for j = 1 : md.ny
          for i = 1 : md.nx
              % elevation
            	fprintf(fid, '%g',  md.elev{ilay}(i,j,k));

              switch md.media_type

              %-- one component
              case 'one_component'
            	  fprintf(fid, ' %g', md.val{ilay}(i,j,k));

              %-- acoustic isotropic
              %   rho Vp
              case 'acoustic_isotropic'
            	  fprintf(fid, ' %g g', ...
            	                md.density{ilay}(i,j,k), ...
                              md.Vp{ilay}(i,j,k));

              %-- elastic isotropic
              %   rho Vp Vs
              case 'elastic_isotropic'
            	  fprintf(fid, ' %g %g %g', ...
            	                md.density{ilay}(i,j,k), ...
                              md.Vp{ilay}(i,j,k), ...
            	                md.Vs{ilay}(i,j,k));

              %-- elastic vti, prem par
              %   rho Vph Vpv Vsh Vsv eta
              case 'elastic_vti_prem'
            	  fprintf(fid, ' %g %g %g %g %g %g', ...
            	                md.density{ilay}(i,j,k), ...
                              md.Vph{ilay}(i,j,k), ...
                              md.Vpv{ilay}(i,j,k), ...
            	                md.Vsh{ilay}(i,j,k),  ...
            	                md.Vsv{ilay}(i,j,k),  ...
            	                md.eta{ilay}(i,j,k) ...
                              );

              %-- elastic vti, thomsen par
              %   rho Vpv Vsv epsilon delta gamma
              case 'elastic_vti_thomsen'
            	  fprintf(fid, ' %g %g %g %g %g %g', ...
            	                md.density{ilay}(i,j,k), ...
                              md.Vpv{ilay}(i,j,k), ...
            	                md.Vsv{ilay}(i,j,k), ... 
            	                md.epsilon{ilay}(i,j,k),  ...
            	                md.detla{ilay}(i,j,k),  ...
            	                md.gamma{ilay}(i,j,k) ...
                              );

              %-- elastic vti, thomsen par
              %   rho c11 c33 c55 c66 c13
              case 'elastic_vti_cij'
            	  fprintf(fid, ' %g %g %g %g %g %g', ...
            	                md.density{ilay}(i,j,k), ...
                              md.C11{ilay}(i,j,k), ...
            	                md.C33{ilay}(i,j,k), ... 
            	                md.C55{ilay}(i,j,k),  ...
            	                md.C66{ilay}(i,j,k),  ...
            	                md.C13{ilay}(i,j,k) ...
                              );

              %-- elastic tti, thomsen par
              %   rho Vpv Vsv epsilon delta gamma azimuth dip
              case 'elastic_tti_thomsen'
            	  fprintf(fid, ' %g %g %g %g %g %g %g %g', ...
            	                md.density{ilay}(i,j,k), ...
                              md.Vpv{ilay}(i,j,k), ...
            	                md.Vsv{ilay}(i,j,k), ... 
            	                md.epsilon{ilay}(i,j,k),  ...
            	                md.detla{ilay}(i,j,k),  ...
            	                md.gamma{ilay}(i,j,k), ...
            	                md.azimuth{ilay}(i,j,k), ...
            	                md.dip{ilay}(i,j,k) ...
                              );

              %-- elastic tti, vti cij plus rotate
              %   rho c11 c33 c55 c66 c13 azimuth dip
              case 'elastic_tti_bond'
            	  fprintf(fid, ' %g %g %g %g %g %g %g %g', ...
            	                md.density{ilay}(i,j,k), ...
                              md.C11{ilay}(i,j,k), ...
            	                md.C33{ilay}(i,j,k), ... 
            	                md.C55{ilay}(i,j,k),  ...
            	                md.C66{ilay}(i,j,k),  ...
            	                md.C13{ilay}(i,j,k), ...
            	                md.azimuth{ilay}(i,j,k), ...
            	                md.dip{ilay}(i,j,k) ...
                              );

              %-- elastic aniso, cij
              %   rho c11 c12 c13 c14 c15 c16 c22 ...
              case 'elastic_aniso_cij'
            	  fprintf(fid, ' %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g %g', ...
            	                md.density{ilay}(i,j,k), ...
                              md.C11{ilay}(i,j,k), ...
                              md.C12{ilay}(i,j,k), ...
                              md.C13{ilay}(i,j,k), ...
                              md.C14{ilay}(i,j,k), ...
                              md.C15{ilay}(i,j,k), ...
                              md.C16{ilay}(i,j,k), ...
                              md.C22{ilay}(i,j,k), ...
                              md.C23{ilay}(i,j,k), ...
                              md.C24{ilay}(i,j,k), ...
                              md.C25{ilay}(i,j,k), ...
                              md.C26{ilay}(i,j,k), ...
                              md.C33{ilay}(i,j,k), ...
                              md.C34{ilay}(i,j,k), ...
                              md.C35{ilay}(i,j,k), ...
                              md.C36{ilay}(i,j,k), ...
                              md.C44{ilay}(i,j,k), ...
                              md.C45{ilay}(i,j,k), ...
                              md.C46{ilay}(i,j,k), ...
                              md.C55{ilay}(i,j,k), ...
                              md.C56{ilay}(i,j,k), ...
                              md.C66{ilay}(i,j,k)  ...
                              );
              end % swith media_type
              
              % return
            	fprintf(fid, '\n');
          end
      end
      end
  end
fclose(fid);

end % function
