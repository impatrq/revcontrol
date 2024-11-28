document.addEventListener("DOMContentLoaded", () => {
    const mobileMenu = document.querySelector('.mobile-menu');
    const nav = document.querySelector('header nav');

    mobileMenu.addEventListener('click', () => {
        nav.classList.toggle('active');
    });

    const navLinks = document.querySelectorAll('nav a');
    navLinks.forEach(link => {
        link.addEventListener('click', (event) => {
            event.preventDefault();
            const targetPage = link.getAttribute('href');
            if (targetPage) {
                window.location.href = targetPage;
            }
        });
    });

    emailjs.init('TU_PUBLIC_KEY'); 

    const form = document.getElementById('contact-form');
    form.addEventListener('submit', (event) => {
        event.preventDefault();

        const formData = new FormData(form);

        const templateParams = {
            name: formData.get('name'),
            email: formData.get('email'),
            phone: formData.get('phone'),
            message: formData.get('message'),
        };

        emailjs.send('TU_SERVICE_ID', 'TU_TEMPLATE_ID', templateParams)
            .then(() => {
                alert("¡Formulario enviado correctamente!");
                form.reset();
            })
            .catch((error) => {
                console.error('Error:', error);
                alert("Hubo un problema al enviar el formulario. Intenta nuevamente.");
            });
    });
});
