/*
Project Name : OpenMEEG

© INRIA and ENPC (contributors: Geoffray ADDE, Maureen CLERC, Alexandre
GRAMFORT, Renaud KERIVEN, Jan KYBIC, Perrine LANDREAU, Théodore PAPADOPOULO,
Emmanuel OLIVI
Maureen.Clerc.AT.inria.fr, keriven.AT.certis.enpc.fr,
kybic.AT.fel.cvut.cz, papadop.AT.inria.fr)

The OpenMEEG software is a C++ package for solving the forward/inverse
problems of electroencephalography and magnetoencephalography.

This software is governed by the CeCILL-B license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL-B
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's authors,  the holders of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-B license and that you accept its terms.
*/

#include <vector.h>
#include <matrix.h>
#include <danielsson.h>
#include <operators.h>
#include <assemble.h>
#include <sensors.h>
#include <GeometryExceptions.H>

#include <constants.h>

namespace OpenMEEG {

    Matrix SurfSourceMat(const Geometry& geo,Mesh& source_mesh,const Integrator& integrator) {

        // Check that there is no overlapping between the geometry and the source mesh.

        if (!geo.check(source_mesh))
            throw OverlappingSourceMesh();

        // The mesh is included in a domain of the geometry.

        const Domain& domain = geo.domain(*source_mesh.vertices().front());

        // Set it as an outermost (to tell _operarorN it doesn't belong to the geometry).

        source_mesh.outermost()       = true;
        source_mesh.current_barrier() = true;

        std::cout << std::endl
                  << "assemble SurfSourceMat with " << source_mesh.vertices().size()
                  << " source_mesh located in domain \"" << domain.name() << "\"." << std::endl
                  << std::endl;

        Matrix mat(geo.nb_parameters()-geo.nb_current_barrier_triangles(),source_mesh.vertices().size());
        mat.set(0.0);

        const double L  = -1.0/domain.conductivity();
        for (const auto& boundary : domain.boundaries()) {
            const double factorN = (boundary.inside()) ? K : -K;
            for (const auto& oriented_mesh : boundary.interface().oriented_meshes()) {
                const Mesh& mesh = oriented_mesh.mesh();

                NonDiagonalBlock operators(mesh,source_mesh,integrator);

                // First block is nVertexFistLayer*source_mesh.vertices().size()
                const double coeffN = factorN*oriented_mesh.orientation();
                operators.set_N_block(coeffN,mat);
                // Second block is nFacesFistLayer*source_mesh.vertices().size()
                operators.D(coeffN*L,mat);
            }
        }

        return mat;
    }

    Matrix
    DipSourceMat(const Geometry& geo,const Matrix& dipoles,const std::string& domain_name,const Integrator& integrator) {

        const size_t size      = geo.nb_parameters()-geo.nb_current_barrier_triangles();
        const size_t n_dipoles = dipoles.nlin();

        Matrix rhs(size,n_dipoles);
        rhs.set(0.0);

        ProgressBar pb(n_dipoles);
        Vector rhs_col(rhs.nlin());
        for (unsigned s=0; s<n_dipoles; ++s,++pb) {
            const Dipole dipole(s,dipoles);
            const Domain domain = (domain_name=="") ? geo.domain(dipole.position()) : geo.domain(domain_name);

            //  Only consider dipoles in non-zero conductivity domain.

            const double cond = domain.conductivity();
            if (cond!=0.0) {
                rhs_col.set(0.0);
                const double K = 1.0/(4*Pi);
                for (const auto& boundary : domain.boundaries()) {
                    const double factorD = (boundary.inside()) ? K : -K;
                    for (const auto& oriented_mesh : boundary.interface().oriented_meshes()) {
                        //  Treat the mesh.
                        const double coeffD = factorD*oriented_mesh.orientation();
                        const Mesh&  mesh   = oriented_mesh.mesh();
                        operatorDipolePotDer(dipole,mesh,rhs_col,coeffD,integrator);

                        if (!oriented_mesh.mesh().current_barrier()) {
                            const double coeff = -coeffD/cond;;
                            operatorDipolePot(dipole,mesh,rhs_col,coeff,integrator);
                        }
                    }
                }
                rhs.setcol(s,rhs_col);
            }
        }
        return rhs;
    }

    Matrix EITSourceMat(const Geometry& geo,const Sensors& electrodes,const Integrator& integrator) {

        // Matrix to be applied to the scalp-injected current to obtain the source term of the EIT foward problem,
        // following article BOUNDARY ELEMENT FORMULATION FOR ELECTRICAL IMPEDANCE TOMOGRAPHY
        // (eq.14 (do not look at eq.16 since there is a mistake: D_23 -> S_23))
        // rhs = [0 ... 0  -D*_23  sigma_3^(-1)S_23  -I_33/2.+D*_33]

        SymMatrix transmat(geo.nb_parameters());
        transmat.set(0.0);

        // This is an overkill. Can we limit the computation only to injection triangles ?
        // We use only the few lines that correspond to injection triangles.

        for (const auto& mp : geo.communicating_mesh_pairs()) {
            const Mesh& mesh1 = mp(0);
            const Mesh& mesh2 = mp(1);

            if (mesh1.current_barrier()) {
                const NonDiagonalBlock operators(mesh1,mesh2,integrator);
                const int orientation = geo.oriented(mesh1,mesh2);
                operators.D(K*orientation,transmat); // D23 or D33 of the formula.
                if (&mesh1==&mesh2) { // I_33 of the formula.
                    DiagonalBlock block(mesh1,integrator);
                    block.addId(-0.5*orientation,transmat);
                } else { // S_2 of the formula.
                    operators.S(-K*orientation*geo.sigma_inv(mesh1,mesh2),transmat);
                }
            }
        }

        const size_t n_sensors = electrodes.getNumberOfSensors();
        Matrix mat(geo.nb_parameters()-geo.nb_current_barrier_triangles(),n_sensors);
        mat.set(0.0);

        for (size_t ielec=0; ielec<n_sensors; ++ielec)
            for (const auto& triangle : electrodes.getInjectionTriangles(ielec)) {
                // To ensure exactly no accumulation of currents. w = electrode_area/triangle_area (~= 1)
                // If no radius is given, we assume the user wants to specify an intensity not a density of current.
                const double coeff = (almost_equal(electrodes.getRadii()(ielec),0.0)) ? 1.0/triangle.area() : electrodes.getWeights()(ielec);
                for (size_t i=0; i<mat.nlin(); ++i)
                    mat(i,ielec) += transmat(triangle.index(),i)*coeff;
            }
        return mat;
    }

    Matrix DipSource2InternalPotMat(const Geometry& geo,const Matrix& dipoles,const Matrix& points,const std::string& domain_name) {

        // Points with one more column for the index of the domain they belong

        std::vector<const Domain*> points_domain;
        std::vector<Vect3>         pts;
        for (unsigned i=0; i<points.nlin(); ++i) {
            const Vect3   point(points(i,0),points(i,1),points(i,2));
            const Domain& domain = geo.domain(point);
            if (domain.conductivity()!=0.0) {
                points_domain.push_back(&domain);
                pts.push_back(point);
            } else {
                std::cerr << " DipSource2InternalPot: Point [ " << points.getlin(i)
                          << "] is outside the head. Point is dropped." << std::endl;
            }
        }

        Matrix mat(pts.size(),dipoles.nlin());
        mat.set(0.0);

        for (unsigned iDIP=0; iDIP<dipoles.nlin(); ++iDIP) {
            const Dipole dipole(iDIP,dipoles);

            const Domain& domain = (domain_name=="") ? geo.domain(dipole.position()) : geo.domain(domain_name);
            const double  coeff  = K/domain.conductivity();

            for (unsigned iPTS=0; iPTS<pts.size(); ++iPTS)
                if (points_domain[iPTS]==&domain)
                    mat(iPTS,iDIP) += coeff*dipole.potential(pts[iPTS]);
        }
        return mat;
    }
}
