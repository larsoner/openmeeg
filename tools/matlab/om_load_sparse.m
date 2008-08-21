function [data] = om_load_sparse(filename,format)

% OM_LOAD_SPARSE   Load sparse Matrix
%
%   Load sparse Matrix
%
%   SYNTAX
%       [DATA] = OM_LOAD_SPARSE(FILENAME,FORMAT)
% 
%       FORMAT : can be 'ascii' or 'binary' (default)
%

% $Id$
% $LastChangedBy$
% $LastChangedDate$
% $Revision$

me = 'OM_LOAD_SPARSE';

if nargin == 0
    eval(['help ',lower(me)])
    return
end

if nargin == 1
    format = 'binary';
end

switch format
case 'binary'
    file = fopen(filename,'r');
    dims = fread(file,2,'uint32','ieee-le');
    data = sparse(dims(1),dims(2));
    while true
        [iijj,count] = fread(file,2,'uint32','ieee-le');
        if count == 0
            break
        end
        [vv,count] = fread(file,1,'double','ieee-le');
        data(iijj(1)+1,iijj(2)+1) = vv;
    end
    fclose(file);
case 'ascii'
    file = fopen(filename,'r');
    dims = fscanf(file, '%d',2);
    data = fscanf(file, '%d %d %f');
    data = reshape(data,3,[])';
    data = sparse(data(:,1)+1,data(:,2)+1,data(:,3),dims(1),dims(2));
    fclose(file);
otherwise
    error([me,' : Unknown file format'])
end
